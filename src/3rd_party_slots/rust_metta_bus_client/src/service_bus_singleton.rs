use std::sync::OnceLock;

use crate::bus::PATTERN_MATCHING_QUERY;
use crate::service_bus::ServiceBus;
use crate::types::BoxError;

static INITIALIZED: OnceLock<bool> = OnceLock::new();
static SERVICE_BUS: OnceLock<ServiceBus> = OnceLock::new();

#[derive(Debug, Default)]
pub struct ServiceBusSingleton;

impl ServiceBusSingleton {
	pub fn init(
		host_id: String, known_peer: String, port_lower: u32, port_upper: u32,
	) -> Result<(), BoxError> {
		INITIALIZED.set(true).expect("ServiceBusSingleton already initialized!");
		let service_bus = ServiceBus::new(
			host_id,
			known_peer,
			vec![PATTERN_MATCHING_QUERY.to_string()],
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
