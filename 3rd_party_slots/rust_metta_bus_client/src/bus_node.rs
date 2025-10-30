use tokio::runtime::Builder;
use tonic::{Request, Status};

use das_proto::{atom_space_node_client::AtomSpaceNodeClient, MessageData};

mod das_proto {
	tonic::include_proto!("dasproto");
}

use crate::{bus::Bus, types::BoxError};

#[derive(Debug, Default, Clone)]
pub struct BusNode {
	pub host_id: String,
	pub peer_id: String,
	pub bus: Bus,
	pub send_join_network: bool,
}

impl BusNode {
	pub fn new(
		host_id: String, commands: Vec<String>, target_id: String,
	) -> Result<Self, BoxError> {
		let mut bus = Bus::new();

		for command in commands {
			bus.add(command.clone());
		}

		Ok(Self { host_id, peer_id: target_id, bus, send_join_network: true })
	}

	pub fn node_id(&self) -> String {
		self.host_id.clone()
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

	pub fn send(
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
				Err(err) => Err(Status::internal(format!("Client failed to connect: {err}"))),
			}
		})?;

		Ok(())
	}
}
