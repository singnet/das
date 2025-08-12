use std::collections::{HashMap, VecDeque};
use std::sync::{Arc, Mutex, RwLock};
use std::thread;

use tokio::runtime::{Builder, Runtime};

use crate::{
	properties::{
		Properties, PropertyValue, ATTENTION_UPDATE_FLAG, COUNT_FLAG, MAX_BUNDLE_SIZE,
		POPULATE_METTA_MAPPING, POSITIVE_IMPORTANCE_FLAG, UNIQUE_ASSIGNMENT_FLAG,
		USE_METTA_AS_QUERY_TOKENS,
	},
	proxy::ProxyNode,
	types::BoxError,
	QueryParams,
};

pub trait BaseQueryProxyT {
	fn finished(&self) -> bool;
	fn pop(&mut self) -> Option<String>;
	fn push(&mut self, query_answer: String) -> Result<(), BoxError>;
	fn get_count(&self) -> u64;
	fn abort(&mut self);
	fn to_remote_peer(&self, command: String, args: Vec<String>) -> Result<(), BoxError>;
	fn setup_proxy_node(
		&mut self, proxy_arc: Arc<Mutex<BaseQueryProxy>>, client_id: Option<String>,
		server_id: Option<String>,
	);
	fn drop_runtime(&mut self);
}

#[derive(Default, Clone)]
pub struct BaseQueryProxy {
	pub answer_queue: VecDeque<String>,
	pub answer_count: u64,

	pub answer_flow_finished: bool,
	pub count_flag: bool,
	pub abort_flag: bool,

	pub query_tokens: Vec<String>,

	// BusNode
	pub service_list: HashMap<String, String>,

	// BusCommandProxy
	pub command: String,
	pub context: String,
	pub properties: Properties,
	pub args: Vec<String>,
	pub requestor_id: String,
	pub serial: u64,
	pub proxy_port: u16,
	pub proxy_node: ProxyNode,

	// Evolution
	pub eval_fitness_queue: VecDeque<String>,

	pub runtime: Arc<RwLock<Option<Arc<Runtime>>>>,
}

impl BaseQueryProxy {
	#[allow(clippy::too_many_arguments)]
	pub fn new(command: String, params: QueryParams) -> Result<Self, BoxError> {
		let mut properties = Properties::new();
		properties.insert(
			UNIQUE_ASSIGNMENT_FLAG.to_string(),
			PropertyValue::Bool(params.unique_assignment),
		);
		properties.insert(
			POSITIVE_IMPORTANCE_FLAG.to_string(),
			PropertyValue::Bool(params.positive_importance),
		);
		properties.insert(
			ATTENTION_UPDATE_FLAG.to_string(),
			PropertyValue::Bool(params.update_attention_broker),
		);
		properties.insert(COUNT_FLAG.to_string(), PropertyValue::Bool(params.count_only));
		properties.insert(
			MAX_BUNDLE_SIZE.to_string(),
			PropertyValue::UnsignedInt(params.max_bundle_size),
		);
		properties.insert(
			POPULATE_METTA_MAPPING.to_string(),
			PropertyValue::Bool(params.populate_metta_mapping),
		);
		properties.insert(
			USE_METTA_AS_QUERY_TOKENS.to_string(),
			PropertyValue::Bool(params.use_metta_as_query_tokens),
		);

		let runtime = Arc::new(RwLock::new(Some(Arc::new(
			Builder::new_multi_thread().enable_all().build().unwrap(),
		))));

		Ok(Self {
			answer_queue: VecDeque::new(),
			answer_count: 0,

			answer_flow_finished: false,
			count_flag: params.count_only,
			abort_flag: false,

			context: params.context,
			properties,

			query_tokens: params.tokens,

			command,
			args: vec![],

			eval_fitness_queue: VecDeque::new(),

			runtime,

			..Default::default()
		})
	}

	pub fn default_with_runtime(runtime: Arc<RwLock<Option<Arc<Runtime>>>>) -> Self {
		Self { runtime, ..Default::default() }
	}
}

impl BaseQueryProxyT for BaseQueryProxy {
	fn finished(&self) -> bool {
		self.abort_flag
			|| (self.answer_flow_finished && (self.count_flag || self.answer_queue.is_empty()))
	}

	fn pop(&mut self) -> Option<String> {
		if self.count_flag {
			log::error!(target: "das", "Can't pop QueryAnswers from count_only queries.");
			return None;
		}
		if self.abort_flag {
			return None;
		}
		self.answer_queue.pop_front()
	}

	fn push(&mut self, query_answer: String) -> Result<(), BoxError> {
		self.answer_queue.push_back(query_answer);
		self.answer_count += 1;
		Ok(())
	}

	fn get_count(&self) -> u64 {
		self.answer_count
	}

	fn abort(&mut self) {
		self.abort_flag = true;
	}

	fn setup_proxy_node(
		&mut self, proxy_arc: Arc<Mutex<BaseQueryProxy>>, client_id: Option<String>,
		server_id: Option<String>,
	) {
		let server_id = server_id.unwrap_or_default();
		if self.proxy_port == 0 {
			panic!("Proxy node can't be set up");
		} else if let Some(client_id) = client_id {
			// This proxy is running in the processor
			self.proxy_node =
				ProxyNode::new(proxy_arc, client_id, server_id.clone(), self.runtime.clone());
		} else {
			// This proxy is running in the requestor
			let id = self.requestor_id.clone();
			let requestor_host = id.split(":").collect::<Vec<_>>()[0];
			let requestor_id = requestor_host.to_string() + ":" + &self.proxy_port.to_string();
			self.proxy_node =
				ProxyNode::new(proxy_arc, requestor_id, server_id, self.runtime.clone());
		}
	}

	fn to_remote_peer(&self, command: String, args: Vec<String>) -> Result<(), BoxError> {
		self.proxy_node.to_remote_peer(command, args)
	}

	fn drop_runtime(&mut self) {
		log::trace!(target: "das", "Dropping BaseQueryProxy...");
		// Releasing Runtime
		let mut runtime_lock = self.runtime.write().unwrap();
		if let Some(runtime) = runtime_lock.take() {
			thread::spawn(move || drop(runtime));
		}
	}
}
