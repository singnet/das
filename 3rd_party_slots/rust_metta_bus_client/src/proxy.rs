use std::net::{SocketAddr, ToSocketAddrs};
use std::sync::{Arc, Mutex, RwLock};

use tokio::runtime::Runtime;
use tonic::{transport::Server, Request, Response, Status};

use das_proto::atom_space_node_client::AtomSpaceNodeClient;
use das_proto::atom_space_node_server::{AtomSpaceNode, AtomSpaceNodeServer};
use das_proto::{Ack, Empty, MessageData};

mod das_proto {
	tonic::include_proto!("dasproto");
}

use crate::base_proxy_query::{BaseQueryProxy, BaseQueryProxyT};
use crate::port_pool::PortPool;
use crate::query_evolution_proxy::EVAL_FITNESS;
use crate::types::BoxError;

static ABORT: &str = "abort"; // Abort current query
static ANSWER_BUNDLE: &str = "answer_bundle"; // Delivery of a bundle with QueryAnswer objects
static COUNT: &str = "count"; // Delivery of the final result of a count_only query
static FINISHED: &str = "finished"; // Notification that all query results have alkready been delivered
static PEER_ERROR: &str = "peer_error"; // Notification that a peer error has occurred

#[derive(Clone, Default)]
pub struct ProxyNode {
	pub node_id: String,
	pub peer_id: String,
	pub runtime: Arc<RwLock<Option<Arc<Runtime>>>>,
}

impl ProxyNode {
	pub fn new(
		proxy: Arc<Mutex<BaseQueryProxy>>, node_id: String, server_id: String,
		runtime: Arc<RwLock<Option<Arc<Runtime>>>>,
	) -> Self {
		StarNode::serve(node_id.clone(), proxy, runtime.clone()).unwrap();
		Self { node_id, peer_id: server_id, runtime: runtime.clone() }
	}

	pub fn node_id(&self) -> String {
		self.node_id.clone()
	}

	pub fn to_remote_peer(&self, command: String, args: Vec<String>) -> Result<(), BoxError> {
		let proxy_command = "bus_command_proxy".to_string();

		let mut new_args = args;
		new_args.push(command);

		log::trace!(target: "das", "ProxyNode::to_remote_peer(): Sending {proxy_command} to {} with args.len={}", self.peer_id, new_args.len());

		let request = Request::new(MessageData {
			command: proxy_command,
			args: new_args,
			sender: self.node_id.clone(),
			is_broadcast: false,
			visited_recipients: vec![],
		});

		let runtime = self.runtime.read().unwrap();
		let runtime = runtime.clone().unwrap();

		let peer_id = self.peer_id.clone();
		runtime.spawn(async move {
			let target_addr = format!("http://{peer_id}");
			match AtomSpaceNodeClient::connect(target_addr).await {
				Ok(mut client) => client.execute_message(request).await,
				Err(err) => {
					log::error!(target: "das", "ProxyNode::to_remote_peer(ERROR): {err:?}");
					Err(Status::internal("Client failed to connect with remote!"))
				},
			}
		});

		Ok(())
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
	proxy: Arc<Mutex<BaseQueryProxy>>,
}

impl StarNode {
	pub fn serve(
		node_id: String, proxy: Arc<Mutex<BaseQueryProxy>>,
		runtime: Arc<RwLock<Option<Arc<Runtime>>>>,
	) -> Result<(), BoxError> {
		let runtime = runtime.read().unwrap();
		let runtime = runtime.clone().unwrap();

		// Start gRPC server (runs indefinitely)
		let node = StarNode { address: StarNode::check_host_id(node_id), proxy };
		runtime.spawn(async move {
			node.start_server().await.unwrap();
		});

		Ok(())
	}

	fn check_host_id(host_id: String) -> SocketAddr {
		match host_id.to_socket_addrs() {
			Ok(mut iter) => iter.next().unwrap(),
			Err(err) => panic!("Can not use '{host_id}' as socket address: {err}"),
		}
	}

	fn process_message(&self, msg: MessageData) -> Result<(), BoxError> {
		log::debug!(
			target: "das",
			"StarNode::process_message()[{}]: MessageData -> command={} | last_arg={:?} | len={} | sender={}",
			self.address, msg.command, msg.args.last(), msg.args.len(), msg.sender
		);

		let mut proxy = self.proxy.lock().unwrap();

		let args = match msg.command.as_str() {
			"node_joined_network" => return Ok(()),
			"query_answer_tokens_flow" => msg.args,
			"query_answer_flow" => return Ok(()),
			"pattern_matching_query" => return Ok(()),
			"query_answers_finished" => return Ok(()),
			"bus_command_proxy" => msg.args,
			"set_command_ownership" => {
				// [command, owner]
				proxy.service_list.insert(msg.args[1].clone(), msg.args[0].clone());
				return Ok(());
			},
			_ => return Ok(()),
		};

		// First, check for early returns and set flags
		let mut eval_fitness_flag = false;
		let mut should_abort = false;

		match args.last() {
			// Peer Error
			Some(last_arg) if last_arg == &PEER_ERROR.to_string() => {
				should_abort = true;
			},
			// Evolution
			Some(last_arg) if last_arg == &EVAL_FITNESS.to_string() => {
				eval_fitness_flag = true;
			},
			_ => {},
		}

		if should_abort {
			proxy.abort_flag = true;
			return Ok(());
		}

		if eval_fitness_flag {
			proxy.proxy_node.peer_id = msg.sender;
			proxy.eval_fitness_queue.extend(args);
			return Ok(());
		}

		for arg in args {
			if arg == FINISHED {
				proxy.answer_flow_finished = true;
				break;
			} else if arg == ABORT {
				proxy.abort_flag = true;
				break;
			} else if arg == ANSWER_BUNDLE || arg == COUNT || arg == EVAL_FITNESS {
				continue;
			}
			proxy.push(arg)?;
		}

		Ok(())
	}
}

#[tonic::async_trait]
impl AtomSpaceNode for StarNode {
	async fn execute_message(
		&self, request: Request<MessageData>,
	) -> Result<Response<Empty>, Status> {
		log::trace!(target: "das", "StarNode::execute_message(): Got MessageData {:?}", request);
		self.process_message(request.into_inner()).map_err(|e| Status::internal(e.to_string()))?;
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
