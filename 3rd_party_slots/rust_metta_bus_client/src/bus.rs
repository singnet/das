use std::collections::HashMap;

pub static PATTERN_MATCHING_QUERY: &str = "pattern_matching_query";
pub static QUERY_EVOLUTION: &str = "query_evolution";

#[derive(Debug, Default, Clone)]
pub struct Bus {
	pub command_owner: HashMap<String, String>,
}

impl Bus {
	pub fn new() -> Self {
		Default::default()
	}

	pub fn add(&mut self, command: String) {
		if let Some(owner) = self.command_owner.get(&command) {
			if !owner.is_empty() {
				panic!("Bus: command <{command}> is already assigned to {owner}");
			}
		} else {
			self.command_owner.insert(command, String::new());
		}
	}

	pub fn set_ownership(&mut self, command: String, node_id: &str) {
		if !self.command_owner.contains_key(&command) {
			panic!("Bus: command <{command}> is not defined");
		} else if self.command_owner[&command].is_empty() {
			self.command_owner.insert(command.to_string(), node_id.to_string());
		} else if self.command_owner[&command] != node_id {
			panic!(
				"Bus: command <{command}> is already assigned to {}",
				self.command_owner[&command]
			);
		}
	}

	pub fn get_ownership(&self, command: String) -> &str {
		match self.command_owner.get(&command) {
			Some(owner) => owner,
			None => panic!("Bus: unknown command <{command}>"),
		}
	}

	pub fn contains(&self, command: String) -> bool {
		self.command_owner.contains_key(&command)
	}
}
