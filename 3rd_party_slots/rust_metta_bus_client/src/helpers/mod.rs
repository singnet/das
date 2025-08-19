pub mod query_answer;

use std::collections::HashSet;

use hyperon_atom::{Atom, VariableAtom};

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

pub fn map_variables(atom_str: &str) -> HashSet<VariableAtom> {
	let mut variables = HashSet::new();

	let tokens = split_ignore_quoted(atom_str);
	for (idx, token) in tokens.iter().enumerate() {
		if token.starts_with("$") {
			let var_name = token.replace("$", "").replace(")", "");
			variables.insert(VariableAtom::parse_name(&var_name).unwrap());
		} else if token == "VARIABLE" && idx + 1 < tokens.len() {
			variables.insert(VariableAtom::new(&tokens[idx + 1]));
		}
	}

	variables
}
