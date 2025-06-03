use tokio::runtime::Builder;
use tonic::{Request, Status};

use das_proto::atom_space_node_client::AtomSpaceNodeClient;
use das_proto::MessageData;

mod das_proto {
	tonic::include_proto!("dasproto");
}

use crate::{bus::Bus, types::BoxError};

#[derive(Debug, Default, Clone)]
pub struct BusNode {
	host_id: String,
	bus: Bus,
}

impl BusNode {
	pub fn new(
		host_id: String, commands: Vec<String>, target_id: String,
	) -> Result<Self, BoxError> {
		let mut bus = Bus::new();
		for command in commands {
			bus.add(command.clone());
			bus.set_ownership(command, &target_id);
		}
		Ok(Self { host_id, bus })
	}

	pub fn node_id(&self) -> String {
		self.host_id.clone()
	}

	pub fn send_bus_command(&self, command: String, args: Vec<String>) -> Result<(), BoxError> {
		let target_id = self.bus.get_ownership(command.clone());
		if target_id.is_empty() {
			log::error!(target: "das", "Bus: no owner is defined for command <{}>", command);
			Err("Bus: no owner is defined for command".into())
		} else {
			log::debug!(target: "das", "BUS node {} is routing command {} to {}", self.node_id(), command, target_id);
			self.send(command, args, target_id.to_string())
		}
	}

	fn send(&self, command: String, args: Vec<String>, target_id: String) -> Result<(), BoxError> {
		let request = Request::new(MessageData {
			command,
			args,
			sender: self.host_id.clone(),
			is_broadcast: false,
			visited_recipients: vec![],
		});

		let runtime = Builder::new_multi_thread().enable_all().build().unwrap();
		runtime.block_on(async move {
			let target_addr = format!("http://{}", target_id);
			log::trace!(target: "das", "BusNode::query(target_addr): {}", target_addr);
			match AtomSpaceNodeClient::connect(target_addr).await {
				Ok(mut client) => client.execute_message(request).await,
				Err(err) => {
					log::error!(target: "das", "BusNode::query(ERROR): {:?}", err);
					Err(Status::internal("Client failed to connect with remote!"))
				},
			}
		})?;

		Ok(())
	}
}
