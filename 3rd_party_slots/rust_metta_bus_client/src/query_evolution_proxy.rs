use std::sync::{Arc, Mutex};

use hyperon_atom::{matcher::Bindings, Atom};

use crate::{
	base_proxy_query::{BaseQueryProxy, BaseQueryProxyT},
	bus::QUERY_EVOLUTION,
	helpers::query_answer::parse_query_answer,
	properties::{
		PropertyValue, ELITISM_RATE, MAX_GENERATIONS, POPULATION_SIZE, SELECTION_RATE,
		TOTAL_ATTENTION_TOKENS,
	},
	types::{BoxError, MeTTaRunner},
	QueryParams,
};

pub static REMOTE_FUNCTION: &str = "remote_fitness_function";
pub static EVAL_FITNESS: &str = "eval_fitness";
pub static EVAL_FITNESS_RESPONSE: &str = "eval_fitness_response";

pub static EVOLUTION_PARSER_ERROR_MESSAGE: &str = "EVOLUTION query must have (parameters) (fitness function) ((query) (correlation tokens) (correlation variables)) eg:
(
	EVOLUTION
	(population_size max_generations elitism_rate selection_rate total_attention_tokens)
	((my-func \"monkey\" $S))
	((Similarity \"human\" $S) (Similarity \"monkey\" $V) ($S $V))
)";

#[derive(Clone)]
pub struct QueryEvolutionParams {
	pub query_atom: Atom,
	pub population_size: u64,
	pub max_generations: u64,
	pub elitism_rate: f64,
	pub selection_rate: f64,
	pub total_attention_tokens: u64,
	pub correlation_tokens: Vec<String>,
	pub correlation_variables: Vec<String>,
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

	pub correlation_tokens: Vec<String>,
	pub correlation_variables: Vec<String>,
}

impl QueryEvolutionProxy {
	pub fn new(
		params: &QueryParams, evolution_params: &QueryEvolutionParams,
	) -> Result<Self, BoxError> {
		let mut base = BaseQueryProxy::new(QUERY_EVOLUTION.to_string(), params.clone())?;

		base.properties.insert(
			POPULATION_SIZE.to_string(),
			PropertyValue::UnsignedInt(evolution_params.population_size),
		);
		base.properties.insert(
			MAX_GENERATIONS.to_string(),
			PropertyValue::UnsignedInt(evolution_params.max_generations),
		);
		base.properties
			.insert(ELITISM_RATE.to_string(), PropertyValue::Double(evolution_params.elitism_rate));
		base.properties.insert(
			SELECTION_RATE.to_string(),
			PropertyValue::Double(evolution_params.selection_rate),
		);
		base.properties.insert(
			TOTAL_ATTENTION_TOKENS.to_string(),
			PropertyValue::UnsignedInt(evolution_params.total_attention_tokens),
		);

		let mut args = vec![];
		args.extend(base.properties.to_vec());
		args.push(base.context.clone());
		args.push(base.query_tokens.len().to_string());
		args.extend(base.query_tokens.clone());
		args.push(REMOTE_FUNCTION.to_string());

		args.push(evolution_params.correlation_tokens.len().to_string());
		args.extend(evolution_params.correlation_tokens.clone());

		args.push(evolution_params.correlation_variables.len().to_string());
		args.extend(evolution_params.correlation_variables.clone());

		log::debug!(target: "das", "Population size       : <{}>", evolution_params.population_size);
		log::debug!(target: "das", "Max generations       : <{}>", evolution_params.max_generations);
		log::debug!(target: "das", "Elitism rate          : <{}>", evolution_params.elitism_rate);
		log::debug!(target: "das", "Selection rate        : <{}>", evolution_params.selection_rate);
		log::debug!(target: "das", "Total attention tokens: <{}>", evolution_params.total_attention_tokens);
		log::debug!(target: "das", "Correlation tokens    : <{}>", evolution_params.correlation_tokens.join(","));
		log::debug!(target: "das", "Correlation variables : <{}>", evolution_params.correlation_variables.join(","));
		log::debug!(target: "das", "Fitness function      : <{}>", evolution_params.fitness_function);

		base.args = args;

		Ok(Self {
			base: Arc::new(Mutex::new(base)),
			population_size: evolution_params.population_size,
			num_generations: 0,
			best_reported_fitness: -1.0,
			ongoing_remote_fitness_evaluation: false,
			correlation_tokens: evolution_params.correlation_tokens.clone(),
			correlation_variables: evolution_params.correlation_variables.clone(),
			fitness_function: evolution_params.fitness_function.clone(),
			metta_runner: if let Some(metta_runner) = evolution_params.maybe_metta_runner.clone() {
				metta_runner
			} else {
				panic!("MeTTa runner is required for evolution query");
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
) -> Result<Option<QueryEvolutionParams>, BoxError> {
	if let Atom::Expression(exp_atom) = &atom {
		let children = exp_atom.children();
		if let Some(Atom::Symbol(s)) = children.first() {
			if s.name() == "EVOLUTION" {
				if children.len() < 4 {
					return Err(EVOLUTION_PARSER_ERROR_MESSAGE.to_string().into());
				}

				// Parameters
				let mut population_size = 50;
				let mut max_generations = 3;
				let mut elitism_rate = 0.05;
				let mut selection_rate = 0.10;
				let mut total_attention_tokens = 100;
				if let Atom::Expression(exp_atom) = children[1].clone() {
					let exp_atom = if let Some(Atom::Expression(inner_exp_atom)) =
						exp_atom.children().first()
					{
						inner_exp_atom.clone()
					} else {
						exp_atom.clone()
					};
					let children = exp_atom.children();
					if children.len() != 5 {
						return Err(EVOLUTION_PARSER_ERROR_MESSAGE.to_string().into());
					}
					population_size = children[0].to_string().parse::<u64>().unwrap();
					max_generations = children[1].to_string().parse::<u64>().unwrap();
					elitism_rate = children[2].to_string().parse::<f64>().unwrap();
					selection_rate = children[3].to_string().parse::<f64>().unwrap();
					total_attention_tokens = children[4].to_string().parse::<u64>().unwrap();
				}

				let mut query_atom = Atom::expr([]);
				let mut correlation_tokens = vec![];
				let mut correlation_variables = vec![];
				if let Atom::Expression(exp_atom) = children[2].clone() {
					let children = exp_atom.children();
					if children.len() != 3 {
						return Err(EVOLUTION_PARSER_ERROR_MESSAGE.to_string().into());
					}
					query_atom = children[0].clone();
					correlation_tokens = vec![children[1].to_string()];
					correlation_variables = match &children[2] {
						Atom::Expression(exp_atom) => exp_atom
							.children()
							.iter()
							.map(|atom| atom.to_string().replace("$", ""))
							.collect(),
						Atom::Symbol(s) => {
							s.name().to_string().split_whitespace().map(String::from).collect()
						},
						_ => vec![],
					};
				}
				let mut fitness_function = children[3].to_string();
				fitness_function = fitness_function[1..fitness_function.len() - 1].to_string();

				return Ok(Some(QueryEvolutionParams {
					query_atom,
					population_size,
					max_generations,
					elitism_rate,
					selection_rate,
					total_attention_tokens,
					correlation_tokens,
					correlation_variables,
					fitness_function,
					maybe_metta_runner: maybe_metta_runner.clone(),
				}));
			}
		}
	}
	Ok(None)
}
