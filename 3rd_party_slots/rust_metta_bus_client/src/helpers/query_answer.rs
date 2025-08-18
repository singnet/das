use std::collections::{HashMap, HashSet};

use hyperon_atom::{matcher::Bindings, Atom, VariableAtom};

const HANDLE_HASH_SIZE: usize = 32;
const MAX_VARIABLE_NAME_SIZE: usize = 100;
const MAX_NUMBER_OF_OPERATION_CLAUSES: usize = 100;
const MAX_NUMBER_OF_VARIABLES_IN_QUERY: usize = 100;

#[derive(Debug, Default, Clone)]
pub struct QueryAnswer {
	pub strength: f64,
	pub importance: f64,
	pub handles: HashSet<String>,
	pub assignment: HashMap<String, String>,
	pub metta_expression: HashMap<String, String>,
}

impl QueryAnswer {
	pub fn untokenize(&mut self, query_answer_str: &str) {
		let token_string = query_answer_str;
		let mut cursor: usize = 0;

		let strength = read_token(token_string, &mut cursor, 13);
		self.strength = strength.parse::<f64>().unwrap_or(0.0);

		let importance = read_token(token_string, &mut cursor, 13);
		self.importance = importance.parse::<f64>().unwrap_or(0.0);

		let number = read_token(token_string, &mut cursor, 4);
		let handles_size = number.parse::<usize>().unwrap_or(0);
		if handles_size > MAX_NUMBER_OF_OPERATION_CLAUSES {
			panic!("Invalid handles_size: {handles_size} untokenizing QueryAnswer");
		}
		for _ in 0..handles_size {
			let handle = read_token(token_string, &mut cursor, HANDLE_HASH_SIZE);
			self.handles.insert(handle);
		}

		let number = read_token(token_string, &mut cursor, 4);
		let assignment_size = number.parse::<usize>().unwrap_or(0);
		if assignment_size > MAX_NUMBER_OF_VARIABLES_IN_QUERY {
			panic!("Invalid number of assignments: {assignment_size} untokenizing QueryAnswer");
		}
		for _ in 0..assignment_size {
			let label = read_token(token_string, &mut cursor, MAX_VARIABLE_NAME_SIZE);
			let handle = read_token(token_string, &mut cursor, HANDLE_HASH_SIZE);
			self.assignment.insert(label, handle);
		}

		let number = read_token(token_string, &mut cursor, 4);
		let metta_mapping_size = number.parse::<usize>().unwrap_or(0);
		if metta_mapping_size > 0 {
			for _ in 0..metta_mapping_size {
				let handle = read_token(token_string, &mut cursor, HANDLE_HASH_SIZE);
				let metta_expression = read_metta_expression(token_string, &mut cursor);
				self.metta_expression.insert(handle, metta_expression);
			}
		}

		if cursor < token_string.len() && !token_string[cursor..].trim().is_empty() {
			panic!("Invalid token string - invalid text after QueryAnswer definition");
		}
	}
}

fn read_token(token_string: &str, cursor: &mut usize, token_size: usize) -> String {
	let bytes = token_string.as_bytes();
	let mut token = Vec::with_capacity(token_size);
	let mut cursor_token = 0;
	while *cursor < bytes.len() && bytes[*cursor] != b' ' {
		if cursor_token == token_size || bytes[*cursor] == b'\0' {
			panic!("Invalid token string");
		}
		token.push(bytes[*cursor]);
		*cursor += 1;
		cursor_token += 1;
	}
	// Skip the space
	if *cursor < bytes.len() && bytes[*cursor] == b' ' {
		*cursor += 1;
	}
	String::from_utf8(token).unwrap_or_default()
}

fn read_metta_expression(token_string: &str, cursor: &mut usize) -> String {
	let bytes = token_string.as_bytes();
	let start = *cursor;
	let mut unmatched = 1;
	let (open_char, close_char) = if bytes[start] == b'(' {
		(b'(', b')')
	} else if bytes[start] == b'"' {
		(b'"', b'"')
	} else {
		(b' ', b' ')
	};
	*cursor += 1;
	while unmatched > 0 {
		if *cursor >= bytes.len() {
			panic!("Invalid metta expression string");
		}
		if bytes[*cursor] == close_char && bytes[*cursor - 1] != b'\\' {
			unmatched -= 1;
		} else if bytes[*cursor] == open_char && bytes[*cursor - 1] != b'\\' {
			unmatched += 1;
		}
		*cursor += 1;
	}
	let end = *cursor;
	// If close_char is not space, skip one more char
	if close_char != b' ' && *cursor < bytes.len() {
		*cursor += 1;
	}
	String::from_utf8(bytes[start..end].to_vec()).unwrap_or_default()
}

pub fn parse_query_answer(query_answer_str: &str, populate_metta_mapping: bool) -> Bindings {
	log::trace!(target: "das", "{query_answer_str}");

	let mut query_answer = QueryAnswer::default();
	query_answer.untokenize(query_answer_str);

	log::trace!(target: "das", "{query_answer:?}");

	let mut bindings = Bindings::new();
	for (key, value) in query_answer.assignment.iter() {
		if populate_metta_mapping {
			if let Some(metta_expression) = query_answer.metta_expression.get(value) {
				let atom_value = Atom::sym(metta_expression.trim().to_string());
				bindings = bindings.add_var_binding(VariableAtom::new(key), atom_value).unwrap();
			}
		} else {
			bindings =
				bindings.add_var_binding(VariableAtom::new(key), Atom::sym(value.clone())).unwrap();
		}
	}

	bindings
}
