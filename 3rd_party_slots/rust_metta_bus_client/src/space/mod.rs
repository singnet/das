use std::{
	fmt::{Debug, Display},
	sync::{Arc, Mutex},
};

use hyperon_atom::{matcher::BindingsSet, Atom};
use hyperon_common::FlexRef;
use hyperon_space::{Space, SpaceCommon, SpaceMut, SpaceVisitor};
use log;

use crate::{
	bus::{CONTEXT_CMD, QUERY_EVOLUTION_CMD},
	compare_networking_params,
	context_broker_proxy::{hash_context, parse_context_broker_parameters},
	create_context, evolution_query,
	helpers::{get_networking_params, run_metta_runner},
	init_service_bus,
	properties::{self, Properties},
	query_evolution_proxy::parse_evolution_parameters,
	query_with_das,
	service_bus_singleton::ServiceBusSingleton,
	types::{BoxError, MeTTaRunner},
	QueryParams, QueryType,
};

#[derive(Clone)]
pub struct DistributedAtomSpace {
	common: SpaceCommon,
	name: Option<String>,
	params: Arc<Mutex<Properties>>,
	maybe_metta_runner: Option<MeTTaRunner>,
}

impl DistributedAtomSpace {
	pub fn new(
		params: Arc<Mutex<Properties>>, maybe_metta_runner: Option<MeTTaRunner>,
	) -> Result<Self, BoxError> {
		let locked_params = params.lock().unwrap().clone();
		let name = Some(locked_params.get(properties::CONTEXT));
		let (host_id, port_lower, port_upper, known_peer) = get_networking_params(&locked_params)?;
		init_service_bus(host_id, known_peer, port_lower, port_upper, false)?;
		Ok(Self { common: SpaceCommon::default(), name, params, maybe_metta_runner })
	}

	pub fn query(&self, query: &Atom) -> BindingsSet {
		match ServiceBusSingleton::get_instance() {
			Ok(service_bus) => {
				let sevice_bus_arc = Arc::new(Mutex::new(service_bus));
				match query_with_das(
					sevice_bus_arc,
					self.params.lock().unwrap().clone(),
					&QueryType::Atom(query.clone()),
				) {
					Ok(bindings) => bindings,
					Err(_) => BindingsSet::empty(),
				}
			},
			Err(_) => BindingsSet::empty(),
		}
	}

	pub fn add(&mut self, _atom: Atom) {
		todo!()
	}

	pub fn remove(&mut self, _atom: &Atom) -> bool {
		todo!()
	}

	pub fn replace(&mut self, _from: &Atom, _to: Atom) -> bool {
		todo!()
	}

	pub fn print_services(&self) -> Result<(), BoxError> {
		let service_bus = ServiceBusSingleton::get_instance()?;
		service_bus.print_services();
		Ok(())
	}

	pub fn join_network(&mut self) -> Result<(), BoxError> {
		let service_bus = ServiceBusSingleton::get_instance()?;
		let host_id = service_bus.host_id.clone();
		let known_peer = service_bus.known_peer.clone();
		let port_lower = service_bus.port_lower;
		let port_upper = service_bus.port_upper;
		let params = self.params.lock().unwrap().clone();
		match compare_networking_params(&host_id, &known_peer, &port_lower, &port_upper, &params) {
			Ok(_) => service_bus.bus_node.lock().unwrap().send_join_network = true,
			Err(_) => {
				drop(service_bus);
				let host_id = params.get::<String>(properties::HOSTNAME);
				let known_peer = params.get::<String>(properties::KNOWN_PEER_ID);
				let port_lower = params.get::<u64>(properties::PORT_LOWER);
				let port_upper = params.get::<u64>(properties::PORT_UPPER);
				init_service_bus(host_id, known_peer, port_lower as u16, port_upper as u16, true)?;
			},
		}
		Ok(())
	}

	pub fn create_context(&self, context: String, atom: &Atom) -> Result<Atom, BoxError> {
		let service_bus = ServiceBusSingleton::get_instance()?;
		let locked_bus = service_bus.bus_node.lock().unwrap().bus.clone();
		if locked_bus.get_ownership(CONTEXT_CMD.to_string()).is_empty() {
			return Err("No context broker available".into());
		}
		let query_params = QueryParams {
			context_name: context.clone(),
			context_key: hash_context(&context),
			properties: self.params.lock().unwrap().clone(),
			tokens: vec![atom.to_string()],
			..Default::default()
		};
		let context_broker_params = parse_context_broker_parameters(atom)?;
		let service_bus_arc = Arc::new(Mutex::new(service_bus));
		create_context(service_bus_arc, &query_params, &context_broker_params)
	}

	pub fn evolution(&self, atom: &Atom) -> Result<BindingsSet, BoxError> {
		let service_bus = ServiceBusSingleton::get_instance()?;
		let locked_bus = service_bus.bus_node.lock().unwrap().bus.clone();
		if locked_bus.get_ownership(QUERY_EVOLUTION_CMD.to_string()).is_empty() {
			return Err("No evolution agent available".into());
		}
		let query_params = QueryParams {
			context_name: self.name.clone().unwrap(),
			context_key: hash_context(&self.name.clone().unwrap()),
			properties: self.params.lock().unwrap().clone(),
			tokens: vec![atom.to_string()],
			..Default::default()
		};

		if let Some(metta_runner) = self.maybe_metta_runner.clone() {
			let atom = run_metta_runner(atom, &metta_runner)?;
			log::debug!(target: "das", "Atom after running metta runner: {atom}");

			let evolution_params = parse_evolution_parameters(&atom, &Some(metta_runner.clone()))?;

			let service_bus_arc = Arc::new(Mutex::new(service_bus));
			return evolution_query(service_bus_arc, &query_params, &evolution_params);
		}
		Err("No metta runner available".into())
	}
}

impl Space for DistributedAtomSpace {
	fn common(&self) -> FlexRef<'_, SpaceCommon> {
		FlexRef::from_simple(&self.common)
	}
	fn query(&self, query: &Atom) -> BindingsSet {
		self.query(query)
	}
	fn atom_count(&self) -> Option<usize> {
		todo!()
	}
	fn visit(&self, _v: &mut dyn SpaceVisitor) -> Result<(), ()> {
		todo!()
	}
	fn subst(&self, pattern: &Atom, template: &Atom) -> Vec<Atom> {
		self.query(pattern)
			.drain(0..)
			.map(|bindings| {
				hyperon_atom::matcher::apply_bindings_to_atom_move(template.clone(), &bindings)
			})
			.collect()
	}
	fn as_any(&self) -> &dyn std::any::Any {
		self
	}
}

impl SpaceMut for DistributedAtomSpace {
	fn add(&mut self, atom: Atom) {
		self.add(atom)
	}
	fn remove(&mut self, atom: &Atom) -> bool {
		self.remove(atom)
	}
	fn replace(&mut self, from: &Atom, to: Atom) -> bool {
		self.replace(from, to)
	}
	fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
		todo!()
	}
}

impl Debug for DistributedAtomSpace {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		match &self.name {
			Some(name) => write!(f, "DistributedAtomSpace-{name} ({self:p})"),
			None => write!(f, "DistributedAtomSpace-{self:p}"),
		}
	}
}

impl Display for DistributedAtomSpace {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		match &self.name {
			Some(name) => write!(f, "DistributedAtomSpace-{name}"),
			None => write!(f, "DistributedAtomSpace-{self:p}"),
		}
	}
}
