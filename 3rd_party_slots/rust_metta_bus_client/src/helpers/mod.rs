pub mod query_answer;

use std::collections::HashMap;

use hyperon_atom::Atom;

use crate::types::{BoxError, MeTTaRunner};

pub fn split_ignore_quoted(s: &str) -> Vec<String> {
	let mut result = Vec::new();
	let chars = s.chars().peekable();
	let mut current = String::new();
	let mut in_single_quotes = false;
	let mut in_double_quotes = false;

	for c in chars {
		match c {
			'\'' if !in_double_quotes && !in_single_quotes => {
				in_single_quotes = true;
				current.push(c);
			},
			'\'' if !in_double_quotes && in_single_quotes => {
				in_single_quotes = false;
				current.push(c);
				result.push(current.clone());
				current.clear();
			},
			'"' if !in_single_quotes && !in_double_quotes => {
				in_double_quotes = true;
				current.push(c);
			},
			'"' if !in_single_quotes && in_double_quotes => {
				in_double_quotes = false;
				current.push(c);
				result.push(current.clone());
				current.clear();
			},
			c if (c == ' ' || c == '\t' || c == '\n') && !in_single_quotes && !in_double_quotes => {
				if !current.is_empty() {
					result.push(current.clone());
					current.clear();
				}
			},
			_ => {
				current.push(c);
			},
		}
	}

	if !current.is_empty() {
		result.push(current);
	}

	result
}

pub fn output_variable_clean_up(atom: &Atom) -> Atom {
	match atom {
		Atom::Variable(v) => {
			if v.name().contains("#") {
				let cleaned_name = v.name().split("#").next().unwrap().to_string();
				Atom::var(cleaned_name)
			} else {
				atom.clone()
			}
		},
		Atom::Expression(exp_atom) => {
			let mut atoms = vec![];
			for atom in exp_atom.children() {
				atoms.push(output_variable_clean_up(atom));
			}
			Atom::expr(atoms)
		},
		_ => atom.clone(),
	}
}

pub fn run_metta_runner(atom: &Atom, metta_runner: &MeTTaRunner) -> Result<Atom, BoxError> {
	match atom {
		Atom::Expression(exp_atom) => {
			let mut atoms = vec![];
			for child in exp_atom.children() {
				let child_str = child.to_string();
				if child_str.contains('!') {
					let result = metta_runner(format!("!{child_str}"))?;
					let mut final_atom = child.clone();
					for inner_atoms in result.iter() {
						for expanded_atom in inner_atoms {
							final_atom = match expanded_atom {
								Atom::Expression(exp_atom) => {
									let mut atoms = vec![];
									for atom in exp_atom.children() {
										match atom {
											Atom::Symbol(s) if s.name() == "!" => {},
											_ => atoms.push(output_variable_clean_up(atom)),
										}
									}
									Atom::expr(atoms)
								},
								_ => expanded_atom.clone(),
							}
						}
					}
					atoms.push(final_atom);
				} else {
					atoms.push(child.clone());
				}
			}
			Ok(Atom::expr(atoms))
		},
		_ => Ok(atom.clone()),
	}
}

pub fn translate_atom(atom: &Atom, variables: &mut HashMap<String, String>) -> Vec<String> {
	match atom {
		Atom::Expression(exp_atom) => {
			let children = exp_atom.children();
			if children.is_empty() {
				return vec!["LINK_TEMPLATE Expression 0".to_string()];
			}

			// Check for AND/OR operators at any level - both Symbol and Grounded
			if let Some(first_child) = children.first() {
				let operator_name = match first_child {
					Atom::Symbol(symbol) => symbol.name().to_string(),
					Atom::Grounded(grounded) => grounded.to_string(),
					_ => "".to_string(),
				};

				if operator_name == "," || operator_name == "and" {
					let mut result = vec![];
					result.push(format!("AND {}", children.len() - 1));

					for child in children.iter().skip(1) {
						let child_tokens = translate_atom(child, variables);
						// Add each child's tokens without indentation
						for token in child_tokens {
							result.push(token);
						}
					}
					result
				} else if operator_name == "or" {
					let mut result = vec![];
					result.push(format!("OR {}", children.len() - 1));

					for child in children.iter().skip(1) {
						let child_tokens = translate_atom(child, variables);
						// Add each child's tokens without indentation
						for token in child_tokens {
							result.push(token);
						}
					}
					result
				} else {
					// Regular expression processing
					let mut result = vec![];

					// Determine if this expression should be LINK or LINK_TEMPLATE
					let mut has_variables = false;
					let mut has_link_template = false;

					// Check all children for variables or LINK_TEMPLATE
					for child in children.iter() {
						let child_tokens = translate_atom(child, variables);
						for token in child_tokens {
							if token.starts_with("VARIABLE") {
								has_variables = true;
							} else if token.starts_with("LINK_TEMPLATE") {
								has_link_template = true;
							}
						}
					}

					let link_type =
						if has_variables || has_link_template { "LINK_TEMPLATE" } else { "LINK" };
					result.push(format!("{} Expression {}", link_type, children.len()));

					for child in children {
						let child_tokens = translate_atom(child, variables);
						// Add each child's tokens without indentation
						for token in child_tokens {
							result.push(token);
						}
					}
					result
				}
			} else {
				// Fallback for expressions without a symbol as first child
				let mut result = vec![];
				result.push(format!("LINK_TEMPLATE Expression {}", children.len()));

				for child in children {
					let child_tokens = translate_atom(child, variables);
					// Add each child's tokens without indentation
					for token in child_tokens {
						result.push(token);
					}
				}
				result
			}
		},
		Atom::Symbol(symbol) => {
			vec![format!("NODE Symbol {}", symbol.name())]
		},
		Atom::Variable(variable) => {
			variables.insert(variable.name().to_string(), "".to_string());
			vec![format!("VARIABLE {}", variable.name())]
		},
		Atom::Grounded(grounded) => {
			// For grounded atoms, represent them as symbols
			vec![format!("NODE Symbol {}", grounded.to_string())]
		},
	}
}
