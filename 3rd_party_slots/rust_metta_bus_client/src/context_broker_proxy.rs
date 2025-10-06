use std::sync::{Arc, Mutex};

use hyperon_atom::Atom;

use crate::{
	base_proxy_query::{BaseQueryProxy, BaseQueryProxyT},
	bus::CONTEXT_CMD,
	helpers::query_element::QueryElement,
	properties::{
		PropertyValue, ENFORCE_CACHE_RECREATION, INITIAL_RENT_RATE,
		INITIAL_SPREADING_RATE_LOWERBOUND, INITIAL_SPREADING_RATE_UPPERBOUND, USE_CACHE,
	},
	types::BoxError,
	QueryParams,
};

// ContextBrokerProxy Commands
pub static ATTENTION_BROKER_SET_PARAMETERS: &str = "attention_broker_set_parameters";
pub static ATTENTION_BROKER_SET_PARAMETERS_FINISHED: &str =
	"attention_broker_set_parameters_finished";
pub static CONTEXT_CREATED: &str = "context_created";
pub static SHUTDOWN: &str = "shutdown";

pub static CONTEXT_BROKER_PARSER_ERROR_MESSAGE: &str = "CONTEXT query must have (parameters) ((query) (determiner schema) (stimulus schema)) eg:
(
	CONTEXT
	(use_cache enforce_cache_recreation initial_rent_rate initial_spreading_rate_lowerbound initial_spreading_rate_upperbound)
	((Similarity \"human\" $S) ((_0 S) ...) (S ...))
)";

#[derive(Clone)]
pub struct ContextBrokerParams {
	pub use_cache: bool,
	pub enforce_cache_recreation: bool,
	pub initial_rent_rate: f64,
	pub initial_spreading_rate_lowerbound: f64,
	pub initial_spreading_rate_upperbound: f64,

	pub query_atom: Atom,

	pub determiner_schema: Vec<(QueryElement, QueryElement)>,
	pub stimulus_schema: Vec<QueryElement>,
}

#[derive(Clone)]
pub struct ContextBrokerProxy {
	pub base: Arc<Mutex<BaseQueryProxy>>,

	pub name: String,
	pub key: String,

	pub use_cache: bool,
	pub enforce_cache_recreation: bool,
	pub initial_rent_rate: f64,
	pub initial_spreading_rate_lowerbound: f64,
	pub initial_spreading_rate_upperbound: f64,

	pub determiner_schema: Vec<(QueryElement, QueryElement)>,
	pub stimulus_schema: Vec<QueryElement>,
}

impl ContextBrokerProxy {
	pub fn new(
		params: &QueryParams, context_broker_params: &ContextBrokerParams,
	) -> Result<Self, BoxError> {
		let mut base = BaseQueryProxy::new(CONTEXT_CMD.to_string(), params.clone())?;

		base.properties
			.insert(USE_CACHE.to_string(), PropertyValue::Bool(context_broker_params.use_cache));
		base.properties.insert(
			ENFORCE_CACHE_RECREATION.to_string(),
			PropertyValue::Bool(context_broker_params.enforce_cache_recreation),
		);
		base.properties.insert(
			INITIAL_RENT_RATE.to_string(),
			PropertyValue::Double(context_broker_params.initial_rent_rate),
		);
		base.properties.insert(
			INITIAL_SPREADING_RATE_LOWERBOUND.to_string(),
			PropertyValue::Double(context_broker_params.initial_spreading_rate_lowerbound),
		);
		base.properties.insert(
			INITIAL_SPREADING_RATE_UPPERBOUND.to_string(),
			PropertyValue::Double(context_broker_params.initial_spreading_rate_upperbound),
		);

		let mut args = vec![];
		args.extend(base.properties.to_vec());
		args.push(base.context.clone());
		args.push(base.query_tokens.len().to_string());
		args.extend(base.query_tokens.clone());

		args.push(context_broker_params.stimulus_schema.len().to_string());
		args.extend(context_broker_params.stimulus_schema.iter().map(|e| e.to_string()));

		args.push(context_broker_params.determiner_schema.len().to_string());
		for (source, target) in context_broker_params.determiner_schema.iter() {
			args.push(source.to_string());
			args.push(target.to_string());
		}

		let name = params.context.clone();
		let key = hash_context(&name);

		args.push(key.clone());
		args.push(name.clone());

		log::debug!(target: "das", "Use cache                        : <{}>", context_broker_params.use_cache);
		log::debug!(target: "das", "Enforce cache recreation         : <{}>", context_broker_params.enforce_cache_recreation);
		log::debug!(target: "das", "Initial rent rate                : <{}>", context_broker_params.initial_rent_rate);
		log::debug!(target: "das", "Initial spreading rate lowerbound: <{}>", context_broker_params.initial_spreading_rate_lowerbound);
		log::debug!(target: "das", "Initial spreading rate upperbound: <{}>", context_broker_params.initial_spreading_rate_upperbound);
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
			use_cache: context_broker_params.use_cache,
			enforce_cache_recreation: context_broker_params.enforce_cache_recreation,
			initial_rent_rate: context_broker_params.initial_rent_rate,
			initial_spreading_rate_lowerbound: context_broker_params
				.initial_spreading_rate_lowerbound,
			initial_spreading_rate_upperbound: context_broker_params
				.initial_spreading_rate_upperbound,
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
	) {
		self.base.lock().unwrap().setup_proxy_node(proxy_arc, client_id, server_id)
	}

	fn to_remote_peer(&self, command: String, args: Vec<String>) -> Result<(), BoxError> {
		self.base.lock().unwrap().to_remote_peer(command, args)
	}

	fn drop_runtime(&mut self) {
		self.base.lock().unwrap().drop_runtime()
	}
}

pub fn hash_context(context: &str) -> String {
	use std::collections::hash_map::DefaultHasher;
	use std::hash::{Hash, Hasher};

	let mut hasher = DefaultHasher::new();
	context.hash(&mut hasher);
	format!("{:x}", hasher.finish())
}

pub fn parse_context_broker_parameters(
	atom: &Atom,
) -> Result<Option<ContextBrokerParams>, BoxError> {
	if let Atom::Expression(exp_atom) = &atom {
		let children = exp_atom.children();
		if let Some(Atom::Symbol(s)) = children.first() {
			if s.name() == "CONTEXT" {
				if children.len() < 3 {
					return Err(CONTEXT_BROKER_PARSER_ERROR_MESSAGE.to_string().into());
				}

				// Parameters
				let mut use_cache = true;
				let mut enforce_cache_recreation = false;
				let mut initial_rent_rate = 0.75;
				let mut initial_spreading_rate_lowerbound = 0.1;
				let mut initial_spreading_rate_upperbound = 0.1;
				if let Atom::Expression(exp_atom) = children[1].clone() {
					let exp_atom = if let Some(Atom::Expression(inner_exp_atom)) =
						exp_atom.children().first()
					{
						inner_exp_atom.clone()
					} else {
						exp_atom.clone()
					};
					let children = exp_atom.children();
					if children.len() != 5 {
						return Err(CONTEXT_BROKER_PARSER_ERROR_MESSAGE.to_string().into());
					}
					use_cache = children[0].to_string().parse::<bool>().unwrap();
					enforce_cache_recreation = children[1].to_string().parse::<bool>().unwrap();
					initial_rent_rate = children[2].to_string().parse::<f64>().unwrap();
					initial_spreading_rate_lowerbound =
						children[3].to_string().parse::<f64>().unwrap();
					initial_spreading_rate_upperbound =
						children[4].to_string().parse::<f64>().unwrap();
				}

				let mut query_atom = Atom::expr([]);
				let mut determiner_schema = vec![];
				let mut stimulus_schema = vec![];
				if let Atom::Expression(exp_atom) = children[2].clone() {
					let children = exp_atom.children();
					if children.len() != 3 {
						return Err(CONTEXT_BROKER_PARSER_ERROR_MESSAGE.to_string().into());
					}
					query_atom = children[0].clone();
					determiner_schema =
						vec![(QueryElement::new_handle(0), QueryElement::new_variable("S"))];
					stimulus_schema = vec![QueryElement::new_variable("S")];
				}

				return Ok(Some(ContextBrokerParams {
					use_cache,
					enforce_cache_recreation,
					initial_rent_rate,
					initial_spreading_rate_lowerbound,
					initial_spreading_rate_upperbound,
					query_atom,
					determiner_schema,
					stimulus_schema,
				}));
			}
		}
	}
	Ok(None)
}

pub fn default_context_broker_params() -> ContextBrokerParams {
	ContextBrokerParams {
		use_cache: true,
		enforce_cache_recreation: false,
		initial_rent_rate: 0.25,
		initial_spreading_rate_lowerbound: 0.50,
		initial_spreading_rate_upperbound: 0.70,
		query_atom: Atom::expr([]),
		determiner_schema: vec![
			(QueryElement::new_handle(0), QueryElement::new_variable("sentence1")),
			(QueryElement::new_variable("sentence1"), QueryElement::new_variable("word1")),
		],
		stimulus_schema: vec![],
	}
}
