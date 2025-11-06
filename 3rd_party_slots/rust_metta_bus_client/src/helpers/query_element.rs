use std::fmt::{Display, Formatter, Result};

use hyperon_atom::Atom;

use crate::types::BoxError;

#[derive(Clone, Debug)]
pub enum QueryElement {
	Handle(u32),
	Variable(String),
}

impl QueryElement {
	pub fn new_handle(element: u32) -> Self {
		Self::Handle(element)
	}
	pub fn new_variable(element: &str) -> Self {
		Self::Variable(element.to_string())
	}
}

impl Display for QueryElement {
	fn fmt(&self, f: &mut Formatter<'_>) -> Result {
		match self {
			Self::Handle(handle) => write!(f, "_{handle}"),
			Self::Variable(variable) => write!(f, "${variable}"),
		}
	}
}

impl TryFrom<&Atom> for QueryElement {
	type Error = BoxError;
	fn try_from(atom: &Atom) -> std::result::Result<Self, Self::Error> {
		match atom {
			Atom::Symbol(s) => Ok(QueryElement::new_handle(s.name().parse::<u32>().unwrap())),
			Atom::Variable(v) => Ok(QueryElement::new_variable(&v.name())),
			Atom::Grounded(g) => {
				Ok(QueryElement::new_handle(g.to_string().parse::<u32>().unwrap()))
			},
			_ => Err("Failed to convert Atom to QueryElement".into()),
		}
	}
}
