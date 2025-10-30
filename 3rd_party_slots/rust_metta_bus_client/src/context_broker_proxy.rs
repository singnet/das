use std::sync::{Arc, Mutex};

use hyperon_atom::Atom;
use md5;

use crate::{
	base_proxy_query::{BaseQueryProxy, BaseQueryProxyT},
	bus::CONTEXT_CMD,
	helpers::{map_atom, query_element::QueryElement},
	properties,
	types::BoxError,
	QueryParams,
};

// ContextBrokerProxy Commands
pub static ATTENTION_BROKER_SET_PARAMETERS: &str = "attention_broker_set_parameters";
pub static ATTENTION_BROKER_SET_PARAMETERS_FINISHED: &str =
	"attention_broker_set_parameters_finished";
pub static CONTEXT_CREATED: &str = "context_created";
pub static SHUTDOWN: &str = "shutdown";

pub static CONTEXT_BROKER_PARSER_ERROR_MESSAGE: &str =
	"CONTEXT query must have ((query) (determiner schema) (stimulus schema)) eg:
(
	((Similarity \"human\" $S) ((_0 $S) ($S $V) ...) ($S $V ...))
)";

#[derive(Clone)]
pub struct ContextBrokerParams {
	pub query_atom: Atom,
	pub determiner_schema: Vec<(QueryElement, QueryElement)>,
	pub stimulus_schema: Vec<QueryElement>,
}

#[derive(Clone)]
pub struct ContextBrokerProxy {
	pub base: Arc<Mutex<BaseQueryProxy>>,

	pub name: String,
	pub key: String,

	pub determiner_schema: Vec<(QueryElement, QueryElement)>,
	pub stimulus_schema: Vec<QueryElement>,
}

impl ContextBrokerProxy {
	pub fn new(
		params: &QueryParams, context_broker_params: &ContextBrokerParams,
	) -> Result<Self, BoxError> {
		let mut base = BaseQueryProxy::new(CONTEXT_CMD.to_string(), params.clone())?;

		let name = params.properties.get::<String>(properties::CONTEXT);
		let key = params.context.clone();

		let mut args = vec![];
		args.extend(params.properties.to_vec());
		args.push(base.context.clone());

		let query_tokens = vec![context_broker_params.query_atom.to_string()];
		args.push(query_tokens.len().to_string());
		args.extend(query_tokens);

		args.push(context_broker_params.stimulus_schema.len().to_string());
		args.extend(context_broker_params.stimulus_schema.iter().map(|e| e.to_string()));

		args.push(context_broker_params.determiner_schema.len().to_string());
		for (source, target) in context_broker_params.determiner_schema.iter() {
			args.push(source.to_string());
			args.push(target.to_string());
		}

		args.push(key.clone());
		args.push(name.clone());

		log::debug!(target: "das", "Context name                     : <{name}>");
		log::debug!(target: "das", "Context key                      : <{key}>");
		log::debug!(target: "das", "Query                            : <{}>", context_broker_params.query_atom);
		log::debug!(target: "das", "Use cache                        : <{}>", params.properties.get::<bool>(properties::USE_CACHE));
		log::debug!(target: "das", "Enforce cache recreation         : <{}>", params.properties.get::<bool>(properties::ENFORCE_CACHE_RECREATION));
		log::debug!(target: "das", "Initial rent rate                : <{}>", params.properties.get::<f64>(properties::INITIAL_RENT_RATE));
		log::debug!(target: "das", "Initial spreading rate lowerbound: <{}>", params.properties.get::<f64>(properties::INITIAL_SPREADING_RATE_LOWERBOUND));
		log::debug!(target: "das", "Initial spreading rate upperbound: <{}>", params.properties.get::<f64>(properties::INITIAL_SPREADING_RATE_UPPERBOUND));
		log::debug!(target: "das", "Determiner schema                : <{}>", context_broker_params.determiner_schema
			.iter()
			.map(|(source, target)| format!("({source}, {target})"))
			.collect::<Vec<String>>().join(","));
		log::debug!(target: "das", "Stimulus schema                  : <{}>", context_broker_params.stimulus_schema.iter().map(|e| e.to_string()).collect::<Vec<String>>().join(","));

		base.args = args;

		Ok(Self {
			base: Arc::new(Mutex::new(base)),
			name,
			key,
			determiner_schema: context_broker_params.determiner_schema.clone(),
			stimulus_schema: context_broker_params.stimulus_schema.clone(),
		})
	}

	pub fn get_name(&self) -> String {
		self.name.clone()
	}

	pub fn get_key(&self) -> String {
		self.key.clone()
	}

	pub fn is_context_created(&self) -> bool {
		self.base.lock().unwrap().context_created
	}
}

impl BaseQueryProxyT for ContextBrokerProxy {
	fn finished(&self) -> bool {
		self.base.lock().unwrap().finished()
	}

	fn pop(&mut self) -> Option<String> {
		self.base.lock().unwrap().pop()
	}

	fn push(&mut self, query_answer: String) -> Result<(), BoxError> {
		self.base.lock().unwrap().push(query_answer)
	}

	fn get_count(&self) -> u64 {
		self.base.lock().unwrap().get_count()
	}

	fn abort(&mut self) {
		self.base.lock().unwrap().abort()
	}

	fn setup_proxy_node(
		&mut self, proxy_arc: Arc<Mutex<BaseQueryProxy>>, client_id: Option<String>,
		server_id: Option<String>,
	) -> Result<(), BoxError> {
		self.base.lock().unwrap().setup_proxy_node(proxy_arc, client_id, server_id)?;
		Ok(())
	}

	fn to_remote_peer(&self, command: String, args: Vec<String>) -> Result<(), BoxError> {
		self.base.lock().unwrap().to_remote_peer(command, args)
	}

	fn drop_runtime(&mut self) {
		self.base.lock().unwrap().drop_runtime()
	}
}

pub fn hash_context(context: &str) -> String {
	format!("{:x}", md5::compute(context.as_bytes()))
}

pub fn parse_context_broker_parameters(atom: &Atom) -> Result<ContextBrokerParams, BoxError> {
	if let Atom::Expression(exp_atom) = &atom {
		let children = exp_atom.children();
		if children.len() < 3 {
			return Err(CONTEXT_BROKER_PARSER_ERROR_MESSAGE.to_string().into());
		}

		let query_atom = children[0].clone();
		let determiner_schema = map_atom(&children[1].clone(), |atom| {
			if let Atom::Expression(exp_atom) = atom {
				let children = exp_atom.children();
				if children.len() != 2 {
					return Err(CONTEXT_BROKER_PARSER_ERROR_MESSAGE.to_string().into());
				}

				let first_element = QueryElement::try_from(&children[0])?;
				let second_element = QueryElement::try_from(&children[1])?;

				Ok((first_element, second_element))
			} else {
				Err(CONTEXT_BROKER_PARSER_ERROR_MESSAGE.to_string().into())
			}
		})?;

		let stimulus_schema = map_atom(&children[2].clone(), |atom| QueryElement::try_from(atom))?;

		return Ok(ContextBrokerParams { query_atom, determiner_schema, stimulus_schema });
	}
	Err(CONTEXT_BROKER_PARSER_ERROR_MESSAGE.to_string().into())
}
