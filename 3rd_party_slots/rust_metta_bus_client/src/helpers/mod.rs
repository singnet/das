pub mod mongodb;
pub mod properties;

use std::collections::VecDeque;

// Enum to represent the parsed S-expression tokens
#[derive(Debug, Clone)]
enum Token {
	Symbol(String),
	Variable(String),
	OpenParen,
	CloseParen,
}

// Enum to represent the AST nodes
#[derive(Debug, Clone)]
enum Node {
	Symbol(String),
	Variable(String),
	Expression(Vec<Node>),
}

// Struct to hold the parser state
struct Parser {
	tokens: VecDeque<Token>,
}

impl Parser {
	fn new(input: &str) -> Self {
		let tokens = tokenize(input);
		Parser { tokens: VecDeque::from(tokens) }
	}

	fn parse(&mut self) -> Vec<Node> {
		let mut nodes = Vec::new();
		while let Some(node) = self.parse_expression() {
			nodes.push(node);
		}
		nodes
	}

	fn parse_expression(&mut self) -> Option<Node> {
		if let Some(token) = self.tokens.pop_front() {
			match token {
				Token::OpenParen => {
					let mut nodes = Vec::new();
					while let Some(next_token) = self.tokens.front() {
						match next_token {
							Token::CloseParen => {
								self.tokens.pop_front(); // Consume closing parenthesis
								return Some(Node::Expression(nodes));
							},
							_ => {
								if let Some(node) = self.parse_expression() {
									nodes.push(node);
								} else {
									return None;
								}
							},
						}
					}
					None // Unmatched parenthesis
				},
				Token::Symbol(s) => Some(Node::Symbol(s)),
				Token::Variable(v) => Some(Node::Variable(v)),
				Token::CloseParen => None, // Unexpected closing parenthesis
			}
		} else {
			None
		}
	}
}

fn tokenize(input: &str) -> Vec<Token> {
	let mut tokens = Vec::new();
	let chars = input.chars().peekable();
	let mut current = String::new();
	let mut in_quotes = false;

	for c in chars {
		match c {
			'"' if !in_quotes => {
				if !current.is_empty() {
					tokens.push(classify_token(&current));
					current.clear();
				}
				in_quotes = true;
				current.push(c);
			},
			'"' if in_quotes => {
				current.push(c);
				tokens.push(Token::Symbol(current.clone()));
				current.clear();
				in_quotes = false;
			},
			'(' if !in_quotes => {
				if !current.is_empty() {
					tokens.push(classify_token(&current));
					current.clear();
				}
				tokens.push(Token::OpenParen);
			},
			')' if !in_quotes => {
				if !current.is_empty() {
					tokens.push(classify_token(&current));
					current.clear();
				}
				tokens.push(Token::CloseParen);
			},
			' ' | '\n' | '\t' if !in_quotes => {
				if !current.is_empty() {
					tokens.push(classify_token(&current));
					current.clear();
				}
			},
			_ => {
				current.push(c);
			},
		}
	}

	if !current.is_empty() {
		tokens.push(classify_token(&current));
	}

	tokens
}

// Classify a string as a Symbol or Variable
fn classify_token(s: &str) -> Token {
	if let Some(s) = s.strip_prefix('$') {
		Token::Variable(s.to_string())
	} else {
		Token::Symbol(s.to_string())
	}
}

// Determine if an expression would be LINK_TEMPLATE (contains a VARIABLE)
fn needs_link_template(nodes: &[Node]) -> bool {
	nodes.iter().any(|node| matches!(node, Node::Variable(_)))
}

// Determine if an expression has a direct child that is a LINK_TEMPLATE
fn has_direct_link_template(nodes: &[Node]) -> bool {
	nodes.iter().any(|node| {
		if let Node::Expression(sub_nodes) = node {
			// Child is LINK_TEMPLATE if it contains a VARIABLE and has no inner expressions with variables
			needs_link_template(sub_nodes)
				&& !sub_nodes.iter().any(|sub_node| {
					if let Node::Expression(inner_nodes) = sub_node {
						needs_link_template(inner_nodes)
					} else {
						false
					}
				})
		} else {
			false
		}
	})
}

// Generate the output string from the AST as a single line
fn generate_output(node: &Node) -> String {
	match node {
		Node::Expression(nodes) => {
			let count = nodes.len();
			let mut parts = Vec::new();
			let has_direct_link_template = has_direct_link_template(nodes);
			// Top-level uses LINK_TEMPLATE2 for direct LINK_TEMPLATE, LINK_TEMPLATE otherwise
			let link_type =
				if has_direct_link_template { "LINK_TEMPLATE2" } else { "LINK_TEMPLATE" };
			parts.push(format!("{} Expression {}", link_type, count));
			for node in nodes {
				parts.push(generate_output_inner(node));
			}
			parts.join(" ")
		},
		_ => generate_output_inner(node),
	}
}

fn is_template_like(node: &Node) -> bool {
	match node {
		Node::Variable(_) => true,
		Node::Expression(nodes) => {
			// If this node contains any VARIABLE or has child expressions that are template-like
			needs_link_template(nodes) || nodes.iter().any(is_template_like)
		},
		_ => false,
	}
}

// Helper function to generate output for nested nodes
fn generate_output_inner(node: &Node) -> String {
	match node {
		Node::Symbol(s) => format!("NODE Symbol {}", s),
		Node::Variable(v) => format!("VARIABLE {}", v),
		Node::Expression(nodes) => {
			let count = nodes.len();
			let mut parts = Vec::new();
			let has_direct_link_template = has_direct_link_template(nodes);
			// LINK_TEMPLATE2 only for direct LINK_TEMPLATE child, LINK_TEMPLATE for VARIABLE, LINK otherwise
			let is_template = is_template_like(&Node::Expression(nodes.clone()));
			let link_type = if has_direct_link_template {
				"LINK_TEMPLATE2"
			} else if is_template {
				"LINK_TEMPLATE"
			} else {
				"LINK"
			};
			parts.push(format!("{} Expression {}", link_type, count));
			for node in nodes {
				parts.push(generate_output_inner(node));
			}
			parts.join(" ")
		},
	}
}

pub fn translate(input: &str) -> String {
	let mut parser = Parser::new(input);
	let ast = parser.parse();
	if ast.is_empty() {
		"Parse error".to_string()
	} else {
		ast.iter().map(generate_output).collect::<Vec<String>>().join(" ")
	}
}

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
