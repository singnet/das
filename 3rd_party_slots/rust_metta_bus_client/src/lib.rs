use std::{
	collections::HashSet,
	sync::{Arc, Mutex},
	thread::sleep,
	time::Duration,
};

use helpers::{map_variables, query_answer::parse_query_answer};
use hyperon_atom::{matcher::BindingsSet, Atom, ExecError, VariableAtom};
use pattern_matching_proxy::PatternMatchingQueryProxy;
use service_bus::ServiceBus;
use service_bus_singleton::ServiceBusSingleton;
use types::BoxError;

use crate::{
	base_proxy_query::BaseQueryProxyT,
	context_broker_proxy::{
		default_context_broker_params, ContextBrokerParams, ContextBrokerProxy,
	},
	helpers::{run_metta_runner, split_ignore_quoted},
	inference_proxy::{parse_inference_parameters, InferenceParams, InferenceProxy},
	properties::{Properties, PropertyValue},
	query_evolution_proxy::{
		parse_evolution_parameters, QueryEvolutionParams, QueryEvolutionProxy,
	},
	types::MeTTaRunner,
};

pub mod space;

pub mod bus;
pub mod bus_node;
pub mod context_broker_proxy;
pub mod helpers;
pub mod inference_proxy;
pub mod pattern_matching_proxy;
pub mod port_pool;
pub mod properties;
pub mod proxy;
pub mod query_evolution_proxy;
pub mod service_bus;
pub mod service_bus_singleton;
pub mod types;

pub mod base_proxy_query;

#[derive(Clone, Default)]
pub struct QueryParams {
	tokens: Vec<String>,
	variables: HashSet<VariableAtom>,
	context: String,
	properties: Properties,
}

#[derive(Clone, Debug)]
pub enum QueryType {
	String(String),
	Atom(Atom),
}

pub fn query_with_das(
	space_name: Option<String>, properties: Properties, service_bus: Arc<Mutex<ServiceBus>>,
	query: &QueryType, maybe_metta_runner: Option<MeTTaRunner>,
) -> Result<BindingsSet, BoxError> {
	let mut atom = match query {
		QueryType::Atom(a) => a.clone(),
		_ => {
			log::error!(target: "das", "Invalid query type: {query:?}");
			return Ok(BindingsSet::empty());
		},
	};

	if let Some(metta_runner) = maybe_metta_runner.clone() {
		atom = run_metta_runner(&atom, &metta_runner)?;
	}

	let (maybe_context_broker_params, maybe_evolution_params) =
		match parse_evolution_parameters(&atom, &maybe_metta_runner) {
			Ok(maybe_evolution_params) => {
				if let Some(evolution_params) = maybe_evolution_params.clone() {
					atom = evolution_params.query_atom;
				}
				let maybe_context_broker_params = Some(default_context_broker_params());
				(maybe_context_broker_params, maybe_evolution_params)
			},
			Err(e) => {
				log::error!(target: "das", "{e}");
				return Ok(BindingsSet::empty());
			},
		};

	let maybe_inference_params = match parse_inference_parameters(&atom) {
		Ok(maybe_inference_params) => maybe_inference_params,
		Err(e) => {
			log::error!(target: "das", "{e}");
			return Ok(BindingsSet::empty());
		},
	};

	let params = match extract_query_params(space_name, query, properties) {
		Ok(params) => params,
		Err(bindings_set) => return Ok(bindings_set),
	};

	match (maybe_context_broker_params, maybe_evolution_params, maybe_inference_params) {
		(Some(context_broker_params), Some(evolution_params), None) => {
			evolution_query(service_bus, &params, &context_broker_params, &evolution_params)
		},
		(None, None, Some(inference_params)) => {
			inference_query(service_bus, &params, &inference_params)
		},
		_ => pattern_matching_query(service_bus, &params),
	}
}

#[allow(clippy::too_many_arguments)]
pub fn extract_query_params(
	space_name: Option<String>, query: &QueryType, properties: Properties,
) -> Result<QueryParams, BindingsSet> {
	let mut params = QueryParams::default();

	// Getting variables from query_tokens_str
	let variables: HashSet<VariableAtom> = match query {
		QueryType::String(s) => map_variables(s),
		QueryType::Atom(atom) => atom
			.iter()
			.filter_type::<&VariableAtom>()
			.collect::<HashSet<&VariableAtom>>()
			.into_iter()
			.cloned()
			.collect(),
	};

	params.context = match space_name {
		Some(name) => name.clone(),
		None => "context".to_string(),
	};

	let query_tokens_str = match query {
		QueryType::String(s) => s.clone(),
		QueryType::Atom(a) => a.to_string(),
	};

	let mut use_metta_as_query_tokens = properties.get(properties::USE_METTA_AS_QUERY_TOKENS);
	params.tokens = if query_tokens_str.starts_with("(") {
		vec![query_tokens_str]
	} else {
		use_metta_as_query_tokens = false;
		split_ignore_quoted(&query_tokens_str)
	};
	params.properties.insert(
		properties::USE_METTA_AS_QUERY_TOKENS.to_string(),
		PropertyValue::Bool(use_metta_as_query_tokens),
	);

	log::debug!(target: "das", "Query: <{:?}>", params.tokens);
	log::debug!(target: "das", "Vars : <{}>", variables.iter().map(|v| v.to_string()).collect::<Vec<String>>().join(","));

	params.variables = variables;
	params.properties = properties;

	Ok(params)
}

pub fn pattern_matching_query(
	service_bus: Arc<Mutex<ServiceBus>>, params: &QueryParams,
) -> Result<BindingsSet, BoxError> {
	let mut bindings_set = BindingsSet::empty();

	let mut proxy = PatternMatchingQueryProxy::new(params)?;

	let mut service_bus = service_bus.lock().unwrap();
	service_bus.issue_bus_command(proxy.base.clone())?;

	let populate_metta_mapping = params.properties.get(properties::POPULATE_METTA_MAPPING);
	let max_query_answers: u64 = params.properties.get(properties::MAX_ANSWERS);

	while !proxy.finished() {
		if let Some(query_answer) = proxy.pop() {
			let mut bindings = parse_query_answer(&query_answer, populate_metta_mapping);
			bindings = bindings.narrow_vars(&params.variables);

			bindings_set.push(bindings);
			if max_query_answers > 0 && bindings_set.len() >= max_query_answers as usize {
				break;
			}
		} else {
			sleep(Duration::from_millis(100));
		}
	}

	log::debug!(target: "das", "BindingsSet(len={}): {:?}", bindings_set.len(), bindings_set);

	proxy.drop_runtime();

	Ok(bindings_set)
}

pub fn evolution_query(
	service_bus: Arc<Mutex<ServiceBus>>, params: &QueryParams,
	context_broker_params: &ContextBrokerParams, evolution_params: &QueryEvolutionParams,
) -> Result<BindingsSet, BoxError> {
	let mut bindings_set = BindingsSet::empty();

	let mut service_bus = service_bus.lock().unwrap();

	let context_proxy = ContextBrokerProxy::new(params, context_broker_params)?;

	service_bus.issue_bus_command(context_proxy.base.clone())?;

	// Wait for ContextBrokerProxy to finish context creation
	log::debug!(target: "das", "Waiting for context creation to finish...");
	while !context_proxy.is_context_created() {
		sleep(Duration::from_millis(100));
	}

	let context_str = context_proxy.get_key();
	log::debug!(target: "das", "Context {context_str} was created");

	let mut proxy = QueryEvolutionProxy::new(params, evolution_params)?;

	service_bus.issue_bus_command(proxy.base.clone())?;

	let populate_metta_mapping = params.properties.get(properties::POPULATE_METTA_MAPPING);
	let max_query_answers: u64 = params.properties.get(properties::MAX_ANSWERS);

	while !proxy.finished() {
		// QueryEvolution
		proxy.eval_fitness()?;
		// PatternMatchingQuery
		if let Some(query_answer) = proxy.pop() {
			let bindings = parse_query_answer(&query_answer, populate_metta_mapping);
			bindings_set.push(bindings);
			if max_query_answers > 0 && bindings_set.len() >= max_query_answers as usize {
				break;
			}
		} else {
			sleep(Duration::from_millis(100));
		}
	}

	log::trace!(target: "das", "BindingsSet(len={}): {:?}", bindings_set.len(), bindings_set);

	proxy.drop_runtime();

	Ok(bindings_set)
}

pub fn inference_query(
	service_bus: Arc<Mutex<ServiceBus>>, params: &QueryParams, inference_params: &InferenceParams,
) -> Result<BindingsSet, BoxError> {
	let mut bindings_set = BindingsSet::empty();

	let mut params = params.clone();

	if !InferenceProxy::request_types().contains(&inference_params.request_type) {
		return Err(
			format!("Invalid inference request type: {}", inference_params.request_type).into()
		);
	}

	params
		.properties
		.insert(properties::USE_METTA_AS_QUERY_TOKENS.to_string(), PropertyValue::Bool(false));
	params
		.properties
		.insert(properties::POPULATE_METTA_MAPPING.to_string(), PropertyValue::Bool(false));

	for idx in 0..inference_params.max_proof_length {
		params.variables.insert(VariableAtom::new(format!("V{idx}")));
	}

	let mut proxy = InferenceProxy::new(&params, inference_params)?;

	let mut service_bus = service_bus.lock().unwrap();
	service_bus.issue_bus_command(proxy.base.clone())?;

	let populate_metta_mapping = params.properties.get(properties::POPULATE_METTA_MAPPING);

	while !proxy.finished() {
		if let Some(query_answer) = proxy.pop() {
			let bindings = parse_query_answer(&query_answer, populate_metta_mapping);
			bindings_set.push(bindings);
		} else {
			sleep(Duration::from_millis(100));
		}
	}

	log::trace!(target: "das", "BindingsSet(len={}): {:?}", bindings_set.len(), bindings_set);

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
