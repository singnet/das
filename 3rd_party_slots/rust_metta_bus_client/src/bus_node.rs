use std::{
	sync::{Arc, Mutex, RwLock},
	thread::sleep,
	time::Duration,
};

use tokio::runtime::Builder;
use tonic::{Request, Status};

use das_proto::{atom_space_node_client::AtomSpaceNodeClient, MessageData};

mod das_proto {
	tonic::include_proto!("dasproto");
}

use crate::port_pool::PortPool;
use crate::{
	base_proxy_query::{BaseQueryProxy, BaseQueryProxyT},
	bus::Bus,
	proxy::StarNode,
	types::BoxError,
};

#[derive(Debug, Default, Clone)]
pub struct BusNode {
	host_id: String,
	peer_id: String,
	bus: Bus,
}

impl BusNode {
	pub fn new(
		host_id: String, commands: Vec<String>, target_id: String,
	) -> Result<Self, BoxError> {
		let mut bus = Bus::new();

		for command in commands {
			bus.add(command.clone());
		}

		Ok(Self { host_id, peer_id: target_id, bus })
	}

	pub fn node_id(&self) -> String {
		self.host_id.clone()
	}

	pub fn join_network(&mut self) -> Result<(), BoxError> {
		let runtime = Arc::new(RwLock::new(Some(Arc::new(
			Builder::new_multi_thread().enable_all().build().unwrap(),
		))));

		let proxy_arc = Arc::new(Mutex::new(BaseQueryProxy::default_with_runtime(runtime.clone())));
		let port = PortPool::get_port();
		let host_id = format!("{}:{}", self.host_id, port);

		StarNode::serve(host_id.clone(), proxy_arc.clone(), runtime.clone())?;

		log::trace!(target: "das", "BusNode::join_network(): Joining network with peer {}", self.peer_id);
		self.send("node_joined_network".to_string(), vec![host_id], self.peer_id.clone(), true)?;

		let mut count = 0;
		loop {
			let proxy = proxy_arc.lock().unwrap();
			sleep(Duration::from_millis(100));

			count += 1;
			if count > 200 {
				log::warn!(
					target: "das",
					"BusNode::join_network(): Unable to get all services ({}/{}) from peer {}",
					proxy.service_list.len(),
					self.bus.command_owner.len(),
					self.peer_id,
				);
				break;
			}

			for (command, owner) in proxy.service_list.iter() {
				log::trace!(target: "das", "BusNode::join_network(): Adding {command} to bus, owner: {owner}");
				self.bus.set_ownership(command.clone(), owner);
			}

			if proxy.service_list.len() == self.bus.command_owner.len() {
				break;
			}
			drop(proxy);
		}

		proxy_arc.lock().unwrap().drop_runtime();
		runtime.write().unwrap().take();

		PortPool::return_port(port);

		Ok(())
	}

	pub fn send_bus_command(&self, command: String, args: Vec<String>) -> Result<(), BoxError> {
		let target_id = self.bus.get_ownership(command.clone());
		if target_id.is_empty() {
			log::error!(target: "das", "Bus: no owner is defined for command <{command}>");
			Err("Bus: no owner is defined for command".into())
		} else {
			log::debug!(target: "das", "BUS node {} is routing command {command} to {target_id}", self.node_id());
			self.send(command, args, target_id.to_string(), false)
		}
	}

	fn send(
		&self, command: String, args: Vec<String>, target_id: String, is_broadcast: bool,
	) -> Result<(), BoxError> {
		let request = Request::new(MessageData {
			command,
			args,
			sender: self.host_id.clone(),
			is_broadcast,
			visited_recipients: vec![],
		});

		let runtime = Builder::new_multi_thread().enable_all().build().unwrap();
		runtime.block_on(async move {
			let target_addr = format!("http://{target_id}");
			log::trace!(target: "das", "BusNode::query(target_addr): {target_addr}");
			match AtomSpaceNodeClient::connect(target_addr).await {
				Ok(mut client) => client.execute_message(request).await,
				Err(err) => {
					log::error!(target: "das", "BusNode::query(ERROR): {err:?}");
					Err(Status::internal("Client failed to connect with remote!"))
				},
			}
		})?;

		Ok(())
	}
}
