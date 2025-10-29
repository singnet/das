use std::{
	fmt::{Debug, Display},
	sync::{Arc, Mutex},
};

use hyperon_atom::{matcher::BindingsSet, Atom};
use hyperon_common::FlexRef;
use hyperon_space::{Space, SpaceCommon, SpaceMut, SpaceVisitor};
use log;

use crate::{
	context_broker_proxy::parse_context_broker_parameters,
	create_context, evolution_query,
	helpers::{get_networking_params, run_metta_runner},
	init_service_bus,
	properties::{self, Properties},
	query_evolution_proxy::parse_evolution_parameters,
	query_with_das,
	service_bus::ServiceBus,
	service_bus_singleton::ServiceBusSingleton,
	types::{BoxError, MeTTaRunner},
	QueryParams, QueryType,
};

#[derive(Clone)]
pub struct DistributedAtomSpace {
	common: SpaceCommon,
	service_bus: Arc<Mutex<ServiceBus>>,
	name: Option<String>,
	params: Arc<Mutex<Properties>>,
	metta_runner: MeTTaRunner,
}

impl DistributedAtomSpace {
	pub fn new(
		params: Arc<Mutex<Properties>>, metta_runner: MeTTaRunner,
	) -> Result<Self, BoxError> {
		let locked_params = params.lock().unwrap().clone();
		let name = Some(locked_params.get(properties::CONTEXT));
		let (host_id, port_lower, port_upper, known_peer) =
			match get_networking_params(&locked_params) {
				Ok(params) => params,
				Err(e) => return Err(e),
			};
		let service_bus = Arc::new(Mutex::new(
			match init_service_bus(host_id, known_peer, port_lower, port_upper) {
				Ok(bus) => bus,
				Err(_) => ServiceBusSingleton::get_instance(),
			},
		));
		Ok(Self { common: SpaceCommon::default(), service_bus, name, params, metta_runner })
	}

	pub fn query(&self, query: &Atom) -> BindingsSet {
		match query_with_das(
			self.name.clone(),
			self.params.lock().unwrap().clone(),
			self.service_bus.clone(),
			&QueryType::Atom(query.clone()),
		) {
			Ok(bindings) => bindings,
			Err(e) => {
				log::error!(target: "das", "Error querying with DAS: {e}");
				BindingsSet::empty()
			},
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

	pub fn create_context(&self, context: String, atom: &Atom) -> Result<Atom, BoxError> {
		let query_params = QueryParams {
			context,
			properties: self.params.lock().unwrap().clone(),
			tokens: vec![atom.to_string()],
			..Default::default()
		};
		let context_broker_params = parse_context_broker_parameters(atom)?;
		create_context(self.service_bus.clone(), &query_params, &context_broker_params)
	}

	pub fn evolution(&self, atom: &Atom) -> Result<BindingsSet, BoxError> {
		let query_params = QueryParams {
			context: self.name.clone().unwrap(),
			properties: self.params.lock().unwrap().clone(),
			tokens: vec![atom.to_string()],
			..Default::default()
		};

		let atom = run_metta_runner(atom, &self.metta_runner)?;
		log::debug!(target: "das", "Atom after running metta runner: {atom}");

		let evolution_params = parse_evolution_parameters(&atom, &Some(self.metta_runner.clone()))?;

		evolution_query(self.service_bus.clone(), &query_params, &evolution_params)
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
