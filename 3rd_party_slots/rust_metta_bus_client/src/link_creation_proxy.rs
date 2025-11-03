use std::sync::{Arc, Mutex};

use hyperon_atom::Atom;

use crate::{
	base_proxy_query::{BaseQueryProxy, BaseQueryProxyT},
	bus::LINK_CREATION_CMD,
	helpers::compute_hash,
	properties::{self, PropertyValue},
	types::BoxError,
	QueryParams,
};

pub static LINK_CREATION_PARSER_ERROR_MESSAGE: &str =
	"LINK_CREATION command must have ((query) (template)) eg:
(
	!(das-link-creation! ((and (PREDICATE %P1) (PREDICATE %P2)) (IMPLICATION %P1 %P2)))
)";

#[derive(Clone)]
pub struct LinkCreationParams {
	pub query: Atom,
	pub templates: Vec<Atom>,
}

#[derive(Clone)]
pub struct LinkCreationProxy {
	pub base: Arc<Mutex<BaseQueryProxy>>,
	pub id: String,
	pub query: Atom,
	pub templates: Vec<Atom>,
}

impl LinkCreationProxy {
	pub fn new(
		params: &QueryParams, link_creation_params: &LinkCreationParams,
	) -> Result<Self, BoxError> {
		let mut base = BaseQueryProxy::new(LINK_CREATION_CMD.to_string(), params.clone())?;

		let context_name = params.properties.get::<String>(properties::CONTEXT);
		let mut properties = params.properties.clone();
		properties.insert(
			properties::CONTEXT.to_string(),
			PropertyValue::String(compute_hash(&context_name)),
		);

		let mut args = vec![];
		args.extend(properties.to_vec());

		args.push(link_creation_params.query.to_string());
		args.extend(link_creation_params.templates.iter().map(|template| template.to_string()));

		let original_id = format!("{}{}", base.proxy_node.peer_id, base.serial);
		let id = compute_hash(&original_id);

		log::debug!(target: "das", "Query              : <{}>", link_creation_params.query);
		log::debug!(target: "das", "Templates          : <{}>", link_creation_params.templates.iter().map(|t| t.to_string()).collect::<Vec<String>>().join(" | "));
		log::debug!(target: "das", "Context (name, key): <{}, {}>", context_name, base.context);
		log::debug!(target: "das", "ID                 : <{id}>");
		log::debug!(target: "das", "Repeat count       : <{}>", params.properties.get::<u64>(properties::REPEAT_COUNT));
		log::debug!(target: "das", "Query interval     : <{}>", params.properties.get::<u64>(properties::QUERY_INTERVAL));
		log::debug!(target: "das", "Query timeout      : <{}>", params.properties.get::<u64>(properties::QUERY_TIMEOUT));
		base.args = args;

		Ok(Self {
			base: Arc::new(Mutex::new(base)),
			id,
			query: link_creation_params.query.clone(),
			templates: link_creation_params.templates.clone(),
		})
	}
}

impl BaseQueryProxyT for LinkCreationProxy {
	fn finished(&self) -> bool {
		self.base.lock().unwrap().finished()
	}

	fn pop(&mut self) -> Option<String> {
		self.base.lock().unwrap().pop()
	}

	fn push(&mut self, query_answer: String) -> Result<(), BoxError> {
		self.base.lock().unwrap().push(query_answer)
	}

	fn get_count(&self) -> u64 {
		self.base.lock().unwrap().get_count()
	}

	fn abort(&mut self) {
		self.base.lock().unwrap().abort()
	}

	fn setup_proxy_node(
		&mut self, proxy_arc: Arc<Mutex<BaseQueryProxy>>, client_id: Option<String>,
		server_id: Option<String>,
	) -> Result<(), BoxError> {
		self.base.lock().unwrap().setup_proxy_node(proxy_arc, client_id, server_id)?;
		Ok(())
	}

	fn to_remote_peer(&self, command: String, args: Vec<String>) -> Result<(), BoxError> {
		self.base.lock().unwrap().to_remote_peer(command, args)
	}

	fn drop_runtime(&mut self) {
		self.base.lock().unwrap().drop_runtime()
	}
}

pub fn parse_link_creation_parameters(atom: &Atom) -> Result<LinkCreationParams, BoxError> {
	if let Atom::Expression(exp_atom) = &atom {
		let children = exp_atom.children();
		if children.len() < 2 {
			return Err(LINK_CREATION_PARSER_ERROR_MESSAGE.to_string().into());
		}

		let query = children[0].clone();
		let templates = children[1..].to_vec();

		return Ok(LinkCreationParams { query, templates });
	}
	Err(LINK_CREATION_PARSER_ERROR_MESSAGE.to_string().into())
}
