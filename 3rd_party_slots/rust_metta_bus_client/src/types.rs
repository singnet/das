use std::{error::Error, sync::Arc};

use hyperon_atom::Atom;

pub type BoxError = Box<dyn Error>;

pub type MeTTaRunnerReturn = Vec<Vec<Atom>>;
pub type MeTTaRunner = Arc<dyn Fn(String) -> Result<MeTTaRunnerReturn, String>>;
