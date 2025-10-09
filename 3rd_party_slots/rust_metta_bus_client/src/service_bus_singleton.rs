use std::sync::OnceLock;

use crate::{
	bus::{
		CONTEXT_CMD, INFERENCE_CMD, LINK_CREATION_CMD, PATTERN_MATCHING_QUERY_CMD,
		QUERY_EVOLUTION_CMD,
	},
	service_bus::ServiceBus,
	types::BoxError,
};

static INITIALIZED: OnceLock<bool> = OnceLock::new();
static SERVICE_BUS: OnceLock<ServiceBus> = OnceLock::new();

#[derive(Debug, Default)]
pub struct ServiceBusSingleton;

impl ServiceBusSingleton {
	pub fn init(
		host_id: String, known_peer: String, port_lower: u16, port_upper: u16,
	) -> Result<(), BoxError> {
		INITIALIZED.set(true).expect("ServiceBusSingleton already initialized!");
		let service_bus = ServiceBus::new(
			host_id,
			known_peer,
			vec![
				PATTERN_MATCHING_QUERY_CMD.to_string(),
				QUERY_EVOLUTION_CMD.to_string(),
				LINK_CREATION_CMD.to_string(),
				INFERENCE_CMD.to_string(),
				// CONTEXT_CMD.to_string(),
			],
			port_lower,
			port_upper,
		)?;
		SERVICE_BUS.set(service_bus).expect("ServiceBusSingleton already initialized!");
		Ok(())
	}

	pub fn get_instance() -> ServiceBus {
		INITIALIZED.get().expect("ServiceBusSingleton not initialized!");
		let service_bus = SERVICE_BUS.get().expect("ServiceBusSingleton not initialized!");
		service_bus.clone()
	}
}
