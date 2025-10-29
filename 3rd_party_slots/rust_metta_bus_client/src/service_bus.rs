use std::{
	sync::{Arc, Mutex, RwLock},
	thread::{self, sleep},
	time::Duration,
};

use tokio::runtime::Builder;

use crate::{
	base_proxy_query::{BaseQueryProxy, BaseQueryProxyT},
	bus_node::BusNode,
	port_pool::PortPool,
	proxy::StarNode,
	types::BoxError,
};

#[derive(Debug, Default, Clone)]
pub struct ServiceBus {
	pub service_list: Vec<String>,
	pub bus_node: Arc<Mutex<BusNode>>,
	pub next_request_serial: u64,
}

impl ServiceBus {
	pub fn new(
		host_id: String, known_peer: String, commands: Vec<String>, port_lower: u16,
		port_upper: u16,
	) -> Result<Self, BoxError> {
		PortPool::initialize_statics(port_lower, port_upper)?;
		let host_id = format!("{}:{}", host_id, PortPool::get_port());
		let bus_node =
			Arc::new(Mutex::new(BusNode::new(host_id.clone(), commands.clone(), known_peer)?));
		Ok(Self { service_list: commands, bus_node, next_request_serial: 0 })
	}

	pub fn issue_bus_command(
		&mut self, proxy_arc: Arc<Mutex<BaseQueryProxy>>,
	) -> Result<(), BoxError> {
		let mut proxy = proxy_arc.lock().unwrap();
		let locked_bus_node = self.bus_node.lock().unwrap();
		log::debug!(target: "das", "{} is issuing BUS command <{}>", locked_bus_node.node_id(), proxy.command);
		proxy.requestor_id = locked_bus_node.node_id();
		self.next_request_serial += 1;
		proxy.serial = self.next_request_serial;
		proxy.proxy_port = PortPool::get_port();
		if proxy.proxy_port == 0 {
			panic!("No port is available to start bus command proxy");
		} else {
			proxy.setup_proxy_node(proxy_arc.clone(), None, None);
			let mut args = Vec::new();
			args.push(proxy.requestor_id.clone());
			args.push(proxy.serial.to_string());
			args.push(proxy.proxy_node.node_id());
			for arg in &proxy.args {
				args.push(arg.clone());
			}
			match locked_bus_node.send_bus_command(proxy.command.clone(), args) {
				Ok(_) => (),
				Err(_) => proxy.answer_flow_finished = true,
			}
		}
		Ok(())
	}

	pub fn service(&self) -> Vec<String> {
		self.service_list.clone()
	}

	pub fn join_network_thread(&mut self) -> Result<(), BoxError> {
		let host_id_clone = self.bus_node.lock().unwrap().host_id.clone();
		let bus_clone = self.bus_node.clone();
		thread::spawn(move || {
			let runtime = Arc::new(RwLock::new(Some(Arc::new(
				Builder::new_multi_thread().enable_all().build().unwrap(),
			))));

			let proxy_arc =
				Arc::new(Mutex::new(BaseQueryProxy::default_with_runtime(runtime.clone())));

			StarNode::serve(host_id_clone.clone(), proxy_arc.clone(), runtime.clone()).unwrap();

			let mut send_join_network = true;
			loop {
				let mut bus_node = bus_clone.lock().unwrap();

				if send_join_network {
					log::debug!(target: "das", "BusNode::join_network(): Joining network with peer {}", bus_node.peer_id);
					bus_node
						.send(
							"node_joined_network".to_string(),
							vec![bus_node.host_id.clone()],
							bus_node.peer_id.clone(),
							true,
						)
						.unwrap();
					send_join_network = false;
					sleep(Duration::from_millis(250));
				}

				let proxy = proxy_arc.lock().unwrap();
				for (command, owner) in proxy.service_list.iter() {
					bus_node.bus.set_ownership(command.clone(), owner);
				}
				drop(proxy);
				drop(bus_node);

				sleep(Duration::from_millis(1_000));
			}
		});
		Ok(())
	}
}
