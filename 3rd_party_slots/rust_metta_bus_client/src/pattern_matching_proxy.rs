use std::sync::{Arc, Mutex};

use crate::{
	base_proxy_query::{BaseQueryProxy, BaseQueryProxyT},
	bus::PATTERN_MATCHING_QUERY_CMD,
	properties,
	types::BoxError,
	QueryParams,
};

#[derive(Default, Clone)]
pub struct PatternMatchingQueryProxy {
	pub base: Arc<Mutex<BaseQueryProxy>>,
}

impl PatternMatchingQueryProxy {
	pub fn new(params: &QueryParams) -> Result<Self, BoxError> {
		let mut base = BaseQueryProxy::new(PATTERN_MATCHING_QUERY_CMD.to_string(), params.clone())?;

		let mut args = vec![];
		args.extend(params.properties.to_vec());
		args.push(base.context.clone());
		args.push(base.query_tokens.len().to_string());
		args.extend(base.query_tokens.clone());

		log::debug!(target: "das", "Query                    : <{}>", base.query_tokens.join(" "));
		log::debug!(target: "das", "Context (name, key)      : <{}, {}>", params.properties.get::<String>(properties::CONTEXT), base.context);
		log::debug!(target: "das", "Max answers              : <{}>", params.properties.get::<u64>(properties::MAX_ANSWERS));
		log::debug!(target: "das", "Update Attention Broker  : <{}>", params.properties.get::<bool>(properties::ATTENTION_UPDATE_FLAG));
		log::debug!(target: "das", "Positive Importance      : <{}>", params.properties.get::<bool>(properties::POSITIVE_IMPORTANCE_FLAG));
		log::debug!(target: "das", "Use metta as query tokens: <{}>", params.properties.get::<bool>(properties::USE_METTA_AS_QUERY_TOKENS));
		log::debug!(target: "das", "Populate metta mapping   : <{}>", params.properties.get::<bool>(properties::POPULATE_METTA_MAPPING));

		base.args = args;

		Ok(Self { base: Arc::new(Mutex::new(base)) })
	}
}

impl BaseQueryProxyT for PatternMatchingQueryProxy {
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

	fn to_remote_peer(&self, command: String, args: Vec<String>) -> Result<(), BoxError> {
		self.base.lock().unwrap().to_remote_peer(command, args)
	}

	fn setup_proxy_node(
		&mut self, proxy_arc: Arc<Mutex<BaseQueryProxy>>, client_id: Option<String>,
		server_id: Option<String>,
	) {
		self.base.lock().unwrap().setup_proxy_node(proxy_arc, client_id, server_id)
	}

	fn drop_runtime(&mut self) {
		self.base.lock().unwrap().drop_runtime()
	}
}
