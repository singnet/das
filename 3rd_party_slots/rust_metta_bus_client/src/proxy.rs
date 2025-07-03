use std::collections::VecDeque;
use std::net::{SocketAddr, ToSocketAddrs};
use std::sync::{Arc, Mutex, RwLock};
use std::{env, thread};

use tokio::runtime::{Builder, Runtime};
use tonic::{transport::Server, Request, Response, Status};

use das_proto::atom_space_node_server::{AtomSpaceNode, AtomSpaceNodeServer};
use das_proto::{Ack, Empty, MessageData};

mod das_proto {
	tonic::include_proto!("dasproto");
}

use crate::helpers::mongodb::MongoRepository;
use crate::port_pool::PortPool;
use crate::properties::{
	Properties, PropertyValue, ATTENTION_UPDATE_FLAG, COUNT_FLAG, MAX_BUNDLE_SIZE,
	POSITIVE_IMPORTANCE_FLAG, UNIQUE_ASSIGNMENT_FLAG,
};
use crate::{bus::PATTERN_MATCHING_QUERY, types::BoxError};

static ABORT: &str = "abort"; // Abort current query
static ANSWER_BUNDLE: &str = "answer_bundle"; // Delivery of a bundle with QueryAnswer objects
static COUNT: &str = "count"; // Delivery of the final result of a count_only query
static FINISHED: &str = "finished"; // Notification that all query results have alkready been delivered

#[derive(Default, Clone)]
pub struct PatternMatchingQueryProxy {
	answer_queue: Arc<Mutex<VecDeque<String>>>,
	answer_count: Arc<Mutex<u64>>,

	pub answer_flow_finished: Arc<Mutex<bool>>,
	count_flag: bool,
	abort_flag: Arc<Mutex<bool>>,

	pub query_tokens: Vec<String>,

	// BusCommandProxy
	pub command: String,
	pub context: String,
	pub properties: Properties,
	pub args: Vec<String>,
	pub requestor_id: String,
	pub serial: u64,
	pub proxy_port: u16,
	pub proxy_node: ProxyNode,

	pub maybe_mongodb_repo: Option<MongoRepository>,

	runtime: Arc<RwLock<Option<Arc<Runtime>>>>,
}

impl PatternMatchingQueryProxy {
	pub fn new(
		tokens: Vec<String>, context: String, unique_assignment: bool, positive_importance: bool,
		update_attention_broker: bool, count_only: bool,
	) -> Result<Self, BoxError> {
		let mut properties = Properties::new();
		properties
			.insert(UNIQUE_ASSIGNMENT_FLAG.to_string(), PropertyValue::Bool(unique_assignment));
		properties
			.insert(POSITIVE_IMPORTANCE_FLAG.to_string(), PropertyValue::Bool(positive_importance));
		properties.insert(
			ATTENTION_UPDATE_FLAG.to_string(),
			PropertyValue::Bool(update_attention_broker),
		);
		properties.insert(COUNT_FLAG.to_string(), PropertyValue::Bool(count_only));
		properties.insert(MAX_BUNDLE_SIZE.to_string(), PropertyValue::UnsignedInt(1000));

		let mut args = vec![];
		args.extend(properties.to_vec());
		args.push(context.clone());
		args.push(tokens.len().to_string());
		args.extend(tokens);

		let runtime = Arc::new(Builder::new_multi_thread().enable_all().build().unwrap());
		let runtime = Arc::new(RwLock::new(Some(runtime)));

		// POC (fetching handle's data from MongoDB, directly)
		let maybe_mongodb_repo = match env::var("MONGODB_URI") {
			Ok(uri) => Some(MongoRepository::new(uri.to_string(), runtime.clone())?),
			Err(_) => None,
		};

		Ok(Self {
			answer_queue: Arc::new(Mutex::new(VecDeque::new())),
			answer_count: Arc::new(Mutex::new(0)),

			answer_flow_finished: Arc::new(Mutex::new(false)),
			count_flag: count_only,
			abort_flag: Arc::new(Mutex::new(false)),

			context,
			properties,

			query_tokens: vec![],

			command: PATTERN_MATCHING_QUERY.to_string(),
			args,

			maybe_mongodb_repo,

			runtime,

			..Default::default()
		})
	}

	pub fn finished(&self) -> bool {
		let aq = self.answer_queue.lock().unwrap();
		let answer_flow_finished = self.answer_flow_finished.lock().unwrap();
		let abort_flag = self.abort_flag.lock().unwrap();
		*abort_flag || (*answer_flow_finished && (self.count_flag || aq.is_empty()))
	}

	pub fn pop(&mut self) -> Option<String> {
		let mut aq = self.answer_queue.lock().unwrap();
		let abort_flag = self.abort_flag.lock().unwrap();
		if self.count_flag {
			log::error!(target: "das", "Can't pop QueryAnswers from count_only queries.");
			return None;
		}
		if *abort_flag {
			return None;
		}
		aq.pop_front()
	}

	pub fn get_count(&self) -> u64 {
		let answer_count = self.answer_count.lock().unwrap();
		*answer_count
	}

	pub fn abort() {
		todo!()
	}

	pub fn setup_proxy_node(&mut self, client_id: Option<String>, server_id: Option<String>) {
		if self.proxy_port == 0 {
			panic!("Proxy node can't be set up");
		} else if let Some(client_id) = client_id {
			// This proxy is running in the processor
			let server_id = server_id.unwrap_or_default();
			self.proxy_node = ProxyNode::new(self, client_id, server_id.clone());
			self.proxy_node.peer_id = server_id;
		} else {
			// This proxy is running in the requestor
			let id = self.requestor_id.clone();
			let requestor_host = id.split(":").collect::<Vec<_>>()[0];
			let requestor_id = requestor_host.to_string() + ":" + &self.proxy_port.to_string();
			self.proxy_node = ProxyNode::new(self, requestor_id, "".to_string());
		}
	}

	pub fn drop_runtime(&mut self) {
		log::trace!(target: "das", "Dropping PatternMatchingQueryProxy...");
		// Releasing Runtime
		let mut runtime_lock = self.runtime.write().unwrap();
		if let Some(runtime) = runtime_lock.take() {
			thread::spawn(move || drop(runtime));
		}
	}
}

#[derive(Clone, Default)]
pub struct ProxyNode {
	node_id: String,
	server_id: String,
	peer_id: String,
}

impl ProxyNode {
	pub fn new(proxy: &mut PatternMatchingQueryProxy, node_id: String, server_id: String) -> Self {
		let star_node = StarNode::new(node_id.clone(), Arc::new(proxy.clone()));

		let runtime_lock = proxy.runtime.read().unwrap();
		let runtime = runtime_lock.clone().unwrap();

		// Start gRPC server (runs indefinitely)
		let node_clone = star_node.clone();
		runtime.spawn(async move {
			node_clone.start_server().await.unwrap();
		});

		Self { node_id, server_id, peer_id: "".to_string() }
	}

	pub fn node_id(&self) -> String {
		self.node_id.clone()
	}

	pub fn server_id(&self) -> String {
		self.server_id.clone()
	}
}

impl Drop for ProxyNode {
	fn drop(&mut self) {
		// Returning Port
		if !self.node_id.is_empty() {
			log::trace!(target: "das", "Dropping ProxyNode with data: {}", self.node_id);
			let splitted = self.node_id.split(":").collect::<Vec<_>>();
			if splitted.len() == 2 {
				let port = splitted[1].parse::<u16>().unwrap();
				PortPool::return_port(port);
			} else {
				log::error!(target: "das", "Failed to parse and return port from: {}", self.node_id);
			}
		}
	}
}

#[derive(Clone)]
pub struct StarNode {
	address: SocketAddr,
	proxy: Arc<PatternMatchingQueryProxy>,
}

impl StarNode {
	pub fn new(node_id: String, proxy: Arc<PatternMatchingQueryProxy>) -> Self {
		Self { address: StarNode::check_host_id(node_id), proxy }
	}

	fn check_host_id(host_id: String) -> SocketAddr {
		match host_id.to_socket_addrs() {
			Ok(mut iter) => iter.next().unwrap(),
			Err(err) => panic!("Can not use '{}' as socket address: {}", host_id, err),
		}
	}

	fn process_message(&self, msg: MessageData) {
		log::debug!(
			target: "das",
			"StarNode::process_message()[{}]: MessageData -> len={:?}",
			self.address,
			msg.args.len()
		);
		let args = match msg.command.as_str() {
			"node_joined_network" => vec![],
			"query_answer_tokens_flow" => msg.args,
			"query_answer_flow" => vec![],
			"pattern_matching_query" => vec![],
			"query_answers_finished" => vec![],
			"bus_command_proxy" => msg.args,
			_ => vec![],
		};

		let mut aq = self.proxy.answer_queue.lock().unwrap();
		let mut answer_count = self.proxy.answer_count.lock().unwrap();
		let mut answer_flow_finished = self.proxy.answer_flow_finished.lock().unwrap();
		let mut abort_flag = self.proxy.abort_flag.lock().unwrap();
		for arg in args {
			if arg == FINISHED {
				*answer_flow_finished = true;
				break;
			} else if arg == ABORT {
				*abort_flag = true;
				break;
			} else if arg == ANSWER_BUNDLE || arg == COUNT {
				continue;
			}
			*answer_count += 1;
			aq.push_back(arg);
		}
	}
}

#[tonic::async_trait]
impl AtomSpaceNode for StarNode {
	async fn execute_message(
		&self, request: Request<MessageData>,
	) -> Result<Response<Empty>, Status> {
		log::trace!(target: "das", "StarNode::execute_message(): Got MessageData {:?}", request);
		self.process_message(request.into_inner());
		Ok(Response::new(Empty {}))
	}

	async fn ping(&self, _request: Request<Empty>) -> Result<Response<Ack>, Status> {
		Ok(Response::new(Ack { error: false, msg: "ack".into() }))
	}
}

#[tonic::async_trait]
pub trait GrpcServer {
	async fn start_server(self) -> Result<(), BoxError>;
}

#[tonic::async_trait]
impl GrpcServer for StarNode {
	async fn start_server(self) -> Result<(), BoxError> {
		let addr = self.address;
		log::debug!(target: "das", "StarNode::start_server(): Inside gRPC server thread at {:?}", addr);
		Server::builder().add_service(AtomSpaceNodeServer::new(self)).serve(addr).await.unwrap();
		Ok(())
	}
}
