use std::sync::{Arc, Mutex};

use crate::base_proxy_query::{BaseQueryProxy, BaseQueryProxyT};
use crate::{bus::PATTERN_MATCHING_QUERY, types::BoxError, QueryParams};

#[derive(Default, Clone)]
pub struct PatternMatchingQueryProxy {
	pub base: Arc<Mutex<BaseQueryProxy>>,
}

impl PatternMatchingQueryProxy {
	pub fn new(params: &QueryParams) -> Result<Self, BoxError> {
		let mut base = BaseQueryProxy::new(
			PATTERN_MATCHING_QUERY.to_string(),
			params.tokens.clone(),
			params.context.clone(),
			params.unique_assignment,
			params.positive_importance,
			params.update_attention_broker,
			params.count_only,
			params.populate_metta_mapping,
			params.max_bundle_size,
		)?;

		let mut args = vec![];
		args.extend(base.properties.to_vec());
		args.push(base.context.clone());
		args.push(base.query_tokens.len().to_string());
		args.extend(base.query_tokens.clone());
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
