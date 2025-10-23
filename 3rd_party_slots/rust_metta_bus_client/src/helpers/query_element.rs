use std::fmt::{Display, Formatter, Result};

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
