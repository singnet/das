use std::{
	collections::HashMap,
	sync::{Arc, Mutex},
	thread::sleep,
	time::Duration,
};

use helpers::{query_answer::parse_query_answer, split_ignore_quoted, translate};
use hyperon_atom::{matcher::BindingsSet, Atom, ExecError};
use proxy::PatternMatchingQueryProxy;
use service_bus::ServiceBus;
use service_bus_singleton::ServiceBusSingleton;
use types::{BoxError, MeTTaRunner};

pub mod space;

pub mod bus;
pub mod bus_node;
pub mod helpers;
pub mod port_pool;
pub mod properties;
pub mod proxy;
pub mod service_bus;
pub mod service_bus_singleton;
pub mod types;

pub fn query_with_das(
	space_name: Option<String>, service_bus: Arc<Mutex<ServiceBus>>, atom: &Atom,
	maybe_metta_runner: Option<MeTTaRunner>,
) -> Result<BindingsSet, BoxError> {
	let unique_assignment = true;
	let positive_importance = false;
	let update_attention_broker = false;
	let count_only = false;
	let populate_metta_mapping = true;
	query(
		space_name,
		service_bus,
		atom,
		unique_assignment,
		positive_importance,
		update_attention_broker,
		count_only,
		populate_metta_mapping,
		maybe_metta_runner,
	)
}

pub fn query(
	space_name: Option<String>, service_bus: Arc<Mutex<ServiceBus>>, atom: &Atom,
	unique_assignment: bool, positive_importance: bool, update_attention_broker: bool,
	count_only: bool, populate_metta_mapping: bool, _maybe_metta_runner: Option<MeTTaRunner>,
) -> Result<BindingsSet, BoxError> {
	let mut bindings_set = BindingsSet::empty();
	// Parsing possible parameters: ((max_query_answers) (query))
	let (max_query_answers, multi_tokens) = match atom {
		Atom::Expression(exp_atom) => {
			let children = exp_atom.children();

			let is_exp = match children.first().unwrap() {
				Atom::Symbol(s) => s.name() == ",",
				Atom::Expression(_) => true,
				_ => return Ok(bindings_set),
			};

			let max_query_answers = 0;

			let mut multi_tokens: Vec<Vec<String>> = vec![];
			if is_exp {
				for atom in children.iter() {
					if atom.to_string() == "," {
						continue;
					}
					multi_tokens
						.push(atom.to_string().split_whitespace().map(String::from).collect());
				}
			} else {
				multi_tokens.push(atom.to_string().split_whitespace().map(String::from).collect());
			}

			(max_query_answers, multi_tokens)
		},
		_ => return Ok(bindings_set),
	};

	// Translating to LT and setting the VARIABLES
	let mut tokens = vec![];
	if multi_tokens.len() > 1 {
		tokens.extend(["AND".to_string(), format!("{}", multi_tokens.len())]);
	}
	let mut variables = HashMap::new();
	for innet_tokens in &multi_tokens {
		for (idx, word) in innet_tokens.clone().iter().enumerate() {
			let _word = word.replace("(", "").replace(")", "");
			if _word.starts_with("$") {
				variables.insert(_word.replace("$", ""), "".to_string());
			} else if _word == "VARIABLE" {
				let var_name = innet_tokens[idx + 1].replace("(", "").replace(")", "");
				variables.insert(var_name, "".to_string());
			}
		}
		let first = innet_tokens[0].replace("(", "");
		match first.trim() {
			"LINK_TEMPLATE" | "AND" | "OR" => {
				let _tokens = innet_tokens.join(" ").replace("(", "").replace(")", "");
				tokens.extend(split_ignore_quoted(&_tokens))
			},
			_ => {
				// Translate MeTTa to LINK_TEMPLATE
				let translation = split_ignore_quoted(&translate(&innet_tokens.join(" ")));
				log::debug!(target: "das", "LT: <{}>", translation.join(" "));
				tokens.extend(translation);
			},
		}
	}

	log::debug!(target: "das", "Query: <{}>", tokens.join(" "));

	// Query's params:
	let context = match space_name {
		Some(name) => name.clone(),
		None => "context".to_string(),
	};

	let mut proxy = PatternMatchingQueryProxy::new(
		tokens,
		context,
		unique_assignment,
		positive_importance,
		update_attention_broker,
		count_only,
		populate_metta_mapping,
	)?;

	let mut service_bus = service_bus.lock().unwrap();
	service_bus.issue_bus_command(&mut proxy)?;

	while !proxy.finished() {
		if let Some(query_answer) = proxy.pop() {
			log::trace!(target: "das", "{query_answer}");

			let bindings = parse_query_answer(&query_answer, populate_metta_mapping);
			bindings_set.push(bindings);

			if max_query_answers > 0 && bindings_set.len() >= max_query_answers {
				break;
			}
		} else {
			sleep(Duration::from_millis(100));
		}
	}

	log::trace!(target: "das", "BindingsSet: {:?} (len={})", bindings_set, bindings_set.len());

	proxy.drop_runtime();

	Ok(bindings_set)
}

pub fn init_service_bus(
	host_id: String, known_peer: String, port_lower: u16, port_upper: u16,
) -> Result<ServiceBus, BoxError> {
	ServiceBusSingleton::init(host_id, known_peer, port_lower, port_upper)?;
	Ok(ServiceBusSingleton::get_instance())
}

pub fn host_id_from_atom(atom: &Atom) -> Result<(String, u16, u16), ExecError> {
	let host_id = atom.to_string().replace("(", "").replace(")", "");
	if let Some((host, port_range_str)) = host_id.split_once(':') {
		if let Some((port_lower_str, port_upper_str)) = port_range_str.split_once('-') {
			let port_lower = port_lower_str.parse::<u16>().unwrap();
			let port_upper = port_upper_str.parse::<u16>().unwrap();
			return Ok((host.to_string(), port_lower, port_upper));
		} else {
			let port = port_range_str.parse::<u16>().unwrap();
			return Ok((host_id, port, port));
		}
	}
	Err(ExecError::from("new-das arguments must be a valid endpoint (eg. 0.0.0.0:42000-42999)"))
}
