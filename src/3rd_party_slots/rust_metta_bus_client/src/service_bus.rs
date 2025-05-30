use crate::bus_node::BusNode;
use crate::port_pool::PortPool;
use crate::proxy::PatternMatchingQueryProxy;
use crate::types::BoxError;

#[derive(Debug, Default, Clone)]
pub struct ServiceBus {
	service_list: Vec<String>,
	bus_node: BusNode,
	next_request_serial: u64,
}

impl ServiceBus {
	pub fn new(
		host_id: String, known_peer: String, commands: Vec<String>, port_lower: u32,
		port_upper: u32,
	) -> Result<Self, BoxError> {
		let bus_node = BusNode::new(host_id, commands.clone(), known_peer)?;
		PortPool::initialize_statics(port_lower, port_upper)?;
		Ok(Self { service_list: commands, bus_node, next_request_serial: 0 })
	}

	pub fn issue_bus_command(
		&mut self, proxy: &mut PatternMatchingQueryProxy,
	) -> Result<(), BoxError> {
		log::debug!(target: "das", "{} is issuing BUS command <{}>", self.bus_node.node_id(), proxy.command);
		proxy.requestor_id = self.bus_node.node_id();
		self.next_request_serial += 1;
		proxy.serial = self.next_request_serial;
		proxy.proxy_port = PortPool::get_port();
		if proxy.proxy_port == 0 {
			panic!("No port is available to start bus command proxy");
		} else {
			proxy.setup_proxy_node(None, None);
			let mut args = Vec::new();
			args.push(proxy.requestor_id.clone());
			args.push(proxy.serial.to_string());
			args.push(proxy.proxy_node.node_id());
			for arg in &proxy.args {
				args.push(arg.clone());
			}
			match self.bus_node.send_bus_command(proxy.command.clone(), args) {
				Ok(_) => (),
				Err(_) => {
					let mut answer_flow_finished = proxy.answer_flow_finished.lock().unwrap();
					*answer_flow_finished = true;
				},
			}
		}
		Ok(())
	}

	// TODO: Avoiding warnings...
	pub fn service(&self) -> Vec<String> {
		self.service_list.clone()
	}
}
