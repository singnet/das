use std::{
	collections::HashMap,
	sync::{Arc, Mutex},
};

use hyperon_atom::{matcher::Bindings, Atom};

use crate::{
	base_proxy_query::{BaseQueryProxy, BaseQueryProxyT},
	bus::QUERY_EVOLUTION_CMD,
	helpers::{map_atom, query_answer::parse_query_answer, query_element::QueryElement},
	properties,
	types::{BoxError, MeTTaRunner},
	QueryParams,
};

pub static REMOTE_FUNCTION: &str = "remote_fitness_function";
pub static EVAL_FITNESS: &str = "eval_fitness";
pub static EVAL_FITNESS_RESPONSE: &str = "eval_fitness_response";

pub static EVOLUTION_PARSER_ERROR_MESSAGE: &str = "EVOLUTION query must have ((query) (fitness function) (correlation queries) (correlation replacements) (correlation mappings)) eg:
(
	((Similarity \"human\" $S1) (my-func \"monkey\" $S1) ((Similarity $S1 $S2) () ...) ((string QE) () ...) ((QE QE) () ...))
)";

#[derive(Clone)]
pub struct QueryEvolutionParams {
	pub query_atom: Atom,

	pub correlation_queries: Vec<Vec<String>>,
	pub correlation_replacements: Vec<HashMap<String, QueryElement>>,
	pub correlation_mappings: Vec<(QueryElement, QueryElement)>,

	pub fitness_function: String,
	pub maybe_metta_runner: Option<MeTTaRunner>,
}

#[derive(Clone)]
pub struct QueryEvolutionProxy {
	pub base: Arc<Mutex<BaseQueryProxy>>,
	pub population_size: u64,
	pub num_generations: u64,
	pub best_reported_fitness: f64,
	pub fitness_function: String,
	pub metta_runner: MeTTaRunner,
	pub ongoing_remote_fitness_evaluation: bool,

	pub correlation_queries: Vec<Vec<String>>,
	pub correlation_replacements: Vec<HashMap<String, QueryElement>>,
	pub correlation_mappings: Vec<(QueryElement, QueryElement)>,
}

impl QueryEvolutionProxy {
	pub fn new(
		params: &QueryParams, evolution_params: &QueryEvolutionParams,
	) -> Result<Self, BoxError> {
		let mut base = BaseQueryProxy::new(QUERY_EVOLUTION_CMD.to_string(), params.clone())?;

		let mut args = vec![];
		args.extend(params.properties.to_vec());

		args.push(base.context_key.clone());

		let query_tokens = vec![evolution_params.query_atom.to_string()];
		args.push(query_tokens.len().to_string());
		args.extend(query_tokens);

		args.push(REMOTE_FUNCTION.to_string());

		args.push(evolution_params.correlation_queries.len().to_string());
		for inner_tokens in evolution_params.correlation_queries.iter() {
			args.push(inner_tokens.len().to_string());
			args.extend(inner_tokens.clone());
		}

		args.push(evolution_params.correlation_replacements.len().to_string());
		for replacements in evolution_params.correlation_replacements.iter() {
			args.push(replacements.len().to_string());
			for (key, value) in replacements.iter() {
				args.push(key.clone());
				args.push(value.to_string());
			}
		}

		args.push(evolution_params.correlation_mappings.len().to_string());
		for (key, value) in evolution_params.correlation_mappings.iter() {
			args.push(key.to_string());
			args.push(value.to_string());
		}

		let population_size = params.properties.get::<u64>(properties::POPULATION_SIZE);

		log::debug!(target: "das", "Query                   : <{}>", evolution_params.query_atom);
		log::debug!(target: "das", "Context (name, key)     : <{}, {}>", base.context_name, base.context_key);
		log::debug!(target: "das", "Population size         : <{population_size}>");
		log::debug!(target: "das", "Max generations         : <{}>", params.properties.get::<u64>(properties::MAX_GENERATIONS));
		log::debug!(target: "das", "Elitism rate            : <{}>", params.properties.get::<f64>(properties::ELITISM_RATE));
		log::debug!(target: "das", "Selection rate          : <{}>", params.properties.get::<f64>(properties::SELECTION_RATE));
		log::debug!(target: "das", "Total attention tokens  : <{}>", params.properties.get::<u64>(properties::TOTAL_ATTENTION_TOKENS));
		log::debug!(target: "das", "Correlation queries     : <{}>", evolution_params.correlation_queries.iter().map(|e| e.join(",")).collect::<Vec<String>>().join(","));
		log::debug!(target: "das", "Correlation replacements: <{}>", evolution_params.correlation_replacements.iter().map(|e| e.iter().map(|(k, v)| format!("({k}, {v})")).collect::<Vec<String>>().join(",")).collect::<Vec<String>>().join(","));
		log::debug!(target: "das", "Correlation mappings    : <{}>", evolution_params.correlation_mappings.iter().map(|e| format!("({}, {})", e.0, e.1)).collect::<Vec<String>>().join(","));
		log::debug!(target: "das", "Fitness function        : <{}>", evolution_params.fitness_function);

		base.args = args;

		Ok(Self {
			base: Arc::new(Mutex::new(base)),
			population_size,
			num_generations: 0,
			best_reported_fitness: -1.0,
			ongoing_remote_fitness_evaluation: false,
			correlation_queries: evolution_params.correlation_queries.clone(),
			correlation_mappings: evolution_params.correlation_mappings.clone(),
			correlation_replacements: evolution_params.correlation_replacements.clone(),
			fitness_function: evolution_params.fitness_function.clone(),
			metta_runner: if let Some(metta_runner) = evolution_params.maybe_metta_runner.clone() {
				metta_runner
			} else {
				return Err(BoxError::from("MeTTa runner is required for evolution query"));
			},
		})
	}

	pub fn eval_fitness(&mut self) -> Result<(), BoxError> {
		let mut base = self.base.lock().unwrap();
		let mut fitness_bundle = vec![];
		while let Some(query_answer_str) = base.eval_fitness_queue.pop_front() {
			if query_answer_str == EVAL_FITNESS {
				continue;
			}
			let query_answer = parse_query_answer(&query_answer_str, true);
			let fitness = self.compute_fitness(
				self.fitness_function.clone(),
				query_answer,
				&self.metta_runner,
			)?;
			fitness_bundle.push(fitness.to_string());
		}
		if !fitness_bundle.is_empty() {
			base.to_remote_peer(EVAL_FITNESS_RESPONSE.to_string(), fitness_bundle)
		} else {
			Ok(())
		}
	}

	fn compute_fitness(
		&self, fitness_function: String, bindings: Bindings, metta_runner: &MeTTaRunner,
	) -> Result<f64, BoxError> {
		let mut fitness = 0.0;
		let mut ff_with_assignments = fitness_function.clone();
		for (key, value) in bindings.iter() {
			ff_with_assignments =
				ff_with_assignments.replace(&format!("${}", key.name()), &value.to_string());
		}
		match (metta_runner)(format!("!{}", ff_with_assignments.clone())) {
			Ok(result) => {
				for outer_atoms in result.iter() {
					for inner_atom in outer_atoms.iter() {
						match inner_atom.to_string().parse::<f64>() {
							Ok(value) => fitness += value,
							Err(e) => {
								log::error!(target: "das", "Invalid fitness value: {inner_atom} | {e}")
							},
						}
					}
				}
			},
			Err(e) => log::error!(target: "das", "Error computing fitness: {e}"),
		}

		log::debug!(target: "das", "Fitness function: {ff_with_assignments} -> {fitness}");
		Ok(fitness)
	}

	fn _count_letter(
		&self, bindings: &Bindings, variable_name: String, letter_to_count: char,
	) -> f64 {
		let mut count = 0;
		let mut value_str = String::new();
		let mut sentence_length = 0;
		for (key, value) in bindings.iter() {
			if key.name() == variable_name {
				value_str = value
					.to_string()
					.replace("\"", "")
					.replace("Sentence ", "")
					.replace("(", "")
					.replace(")", "");
				sentence_length = value_str.replace(" ", "").len() as u64;
				let split_value = value_str.split(" ").collect::<Vec<&str>>();
				for tokens in split_value.iter() {
					for token in tokens.chars() {
						if token == letter_to_count {
							count += 1;
						}
					}
				}
				break;
			}
		}
		let value =
			if sentence_length == 0 { 0.0 } else { (count as f64) / (sentence_length as f64) };

		log::debug!(target: "das", "Value string: {value_str} | length: {sentence_length} -> {value:0.4}");
		value
	}
}

impl BaseQueryProxyT for QueryEvolutionProxy {
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
	) {
		self.base.lock().unwrap().setup_proxy_node(proxy_arc, client_id, server_id)
	}

	fn to_remote_peer(&self, command: String, args: Vec<String>) -> Result<(), BoxError> {
		self.base.lock().unwrap().to_remote_peer(command, args)
	}

	fn drop_runtime(&mut self) {
		self.base.lock().unwrap().drop_runtime()
	}
}

pub fn parse_evolution_parameters(
	atom: &Atom, maybe_metta_runner: &Option<MeTTaRunner>,
) -> Result<QueryEvolutionParams, BoxError> {
	if let Atom::Expression(exp_atom) = &atom {
		let children = exp_atom.children();
		if children.len() < 5 {
			return Err(EVOLUTION_PARSER_ERROR_MESSAGE.to_string().into());
		}

		let query_atom = children[0].clone();

		let fitness_function = children[1].to_string();

		let correlation_queries =
			map_atom(&children[2].clone(), |atom| Ok(vec![atom.to_string()]))?;

		let correlation_replacements = map_atom(&children[3].clone(), |atom| {
			if let Atom::Expression(exp_atom) = atom {
				let children = exp_atom.children();
				let mut replacements = HashMap::new();
				if children.len() != 2 {
					return Err(EVOLUTION_PARSER_ERROR_MESSAGE.to_string().into());
				}
				replacements.insert(
					children[0].to_string(),
					QueryElement::new_variable(&children[1].to_string()),
				);
				Ok(replacements)
			} else {
				Err(EVOLUTION_PARSER_ERROR_MESSAGE.to_string().into())
			}
		})?;

		let correlation_mappings = map_atom(&children[4].clone(), |atom| {
			if let Atom::Expression(exp_atom) = atom {
				let children = exp_atom.children();
				if children.len() != 2 {
					return Err(EVOLUTION_PARSER_ERROR_MESSAGE.to_string().into());
				}
				Ok((
					QueryElement::new_variable(&children[0].to_string()),
					QueryElement::new_variable(&children[1].to_string()),
				))
			} else {
				Err(EVOLUTION_PARSER_ERROR_MESSAGE.to_string().into())
			}
		})?;

		return Ok(QueryEvolutionParams {
			query_atom,
			correlation_queries,
			correlation_mappings,
			correlation_replacements,
			fitness_function,
			maybe_metta_runner: maybe_metta_runner.clone(),
		});
	}
	Err(EVOLUTION_PARSER_ERROR_MESSAGE.to_string().into())
}
