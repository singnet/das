use std::sync::{Arc, Mutex};

use hyperon_atom::Atom;

use crate::{
	base_proxy_query::{BaseQueryProxy, BaseQueryProxyT},
	bus::INFERENCE_CMD,
	helpers::split_ignore_quoted,
	types::BoxError,
	QueryParams,
};

pub const PROOF_OF_IMPLICATION_OR_EQUIVALENCE: &str = "PROOF_OF_IMPLICATION_OR_EQUIVALENCE";
pub const PROOF_OF_IMPLICATION: &str = "PROOF_OF_IMPLICATION";
pub const PROOF_OF_EQUIVALENCE: &str = "PROOF_OF_EQUIVALENCE";

pub static INFERENCE_PARSER_ERROR_MESSAGE: &str = "INFERENCE query must have (request_type handle1 handle2 max_proof_length) eg:
(
	INFERENCE
	(PROOF_OF_IMPLICATION_OR_EQUIVALENCE cf07db895a5656bfd3652ba565727554 c4d0ef92e7265f9298f8dd6d3ae1e014 1)
)";

#[derive(Clone)]
pub struct InferenceParams {
	pub request_type: String,
	pub handle1: String,
	pub handle2: String,
	pub max_proof_length: u64,
}

#[derive(Clone)]
pub struct InferenceProxy {
	pub base: Arc<Mutex<BaseQueryProxy>>,
}

impl InferenceProxy {
	pub fn new(params: &QueryParams, inference_params: &InferenceParams) -> Result<Self, BoxError> {
		let mut base = BaseQueryProxy::new(INFERENCE_CMD.to_string(), params.clone())?;

		let mut args = vec![];
		args.extend(base.properties.to_vec());
		args.push(base.context.clone());
		// TODO(arturgontijo): Shouldn't be the query length ?!
		args.push("0".to_string());
		args.push(inference_params.request_type.clone());
		args.push(inference_params.handle1.clone());
		args.push(inference_params.handle2.clone());
		args.push(inference_params.max_proof_length.to_string());
		// TODO(arturgontijo): Context (bug) ?!
		args.push(base.context.clone());
		base.args = args;

		Ok(InferenceProxy { base: Arc::new(Mutex::new(base)) })
	}

	pub fn request_types() -> Vec<String> {
		vec![
			PROOF_OF_IMPLICATION_OR_EQUIVALENCE.to_string(),
			PROOF_OF_IMPLICATION.to_string(),
			PROOF_OF_EQUIVALENCE.to_string(),
		]
	}
}

impl BaseQueryProxyT for InferenceProxy {
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

	fn to_remote_peer(&self, command: String, args: Vec<String>) -> Result<(), BoxError> {
		self.base.lock().unwrap().to_remote_peer(command, args)
	}

	fn setup_proxy_node(
		&mut self, proxy_arc: Arc<Mutex<BaseQueryProxy>>, client_id: Option<String>,
		server_id: Option<String>,
	) {
		self.base.lock().unwrap().setup_proxy_node(proxy_arc, client_id, server_id)
	}

	fn drop_runtime(&mut self) {
		self.base.lock().unwrap().drop_runtime()
	}
}

pub fn parse_inference_parameters(atom: &Atom) -> Result<Option<InferenceParams>, BoxError> {
	if let Atom::Expression(exp_atom) = &atom {
		let children = exp_atom.children();
		if let Some(Atom::Symbol(s)) = children.first() {
			if s.name() == "INFERENCE" {
				if children.len() < 2 {
					return Err(INFERENCE_PARSER_ERROR_MESSAGE.to_string().into());
				}
				let tokens = children[1].to_string().replace("(", "").replace(")", "");
				let tokens = split_ignore_quoted(&tokens);
				return Ok(Some(InferenceParams {
					request_type: tokens[0].clone(),
					handle1: tokens[1].clone(),
					handle2: tokens[2].clone(),
					max_proof_length: tokens[3].parse().unwrap(),
				}));
			}
		}
	}
	Ok(None)
}
