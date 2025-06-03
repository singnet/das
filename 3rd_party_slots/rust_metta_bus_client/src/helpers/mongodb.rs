use std::{
	future::Future,
	pin::Pin,
	sync::{Arc, RwLock},
	time::Duration,
};

use mongodb::{
	bson::{doc, Document},
	options::ClientOptions,
	Client, Collection,
};

use serde::{Deserialize, Serialize};
use tokio::runtime::Runtime;

use crate::types::BoxError;

// Struct to represent the MongoDB document (for deserialization)
#[derive(Debug, Serialize, Deserialize)]
struct TargetDoc {
	#[serde(rename = "_id")]
	id: String,
	#[serde(default)]
	name: Option<String>,
	#[serde(default)]
	targets: Vec<String>,
}

#[derive(Clone)]
pub struct MongoRepository {
	collection: Collection<Document>,
	runtime: Arc<RwLock<Option<Arc<Runtime>>>>,
}

impl MongoRepository {
	pub fn new(uri: String, runtime: Arc<RwLock<Option<Arc<Runtime>>>>) -> Result<Self, BoxError> {
		let runtime_lock = runtime.read().unwrap();
		let rt = runtime_lock.as_ref().unwrap();

		let client = rt.block_on(async move {
			let mut client_options = ClientOptions::parse(&uri).await?;
			client_options.server_selection_timeout = Some(Duration::from_secs(60));
			Client::with_options(client_options)
		})?;

		let db = client.database("das");
		let collection: Collection<Document> = db.collection("atoms");

		Ok(MongoRepository { collection, runtime: runtime.clone() })
	}

	pub fn fetch_handle_name(&self, id: &str) -> Result<String, BoxError> {
		let runtime_lock = self.runtime.read().unwrap();
		let rt = runtime_lock.as_ref().unwrap();
		rt.block_on(self.fetch_handle_name_async(id))
	}

	fn fetch_handle_name_async<'a>(
		&'a self, id: &'a str,
	) -> Pin<Box<dyn Future<Output = Result<String, BoxError>> + Send + 'a>> {
		Box::pin(async move {
			let filter = doc! { "_id": id };
			if let Some(doc) = self.collection.find_one(filter).await? {
				let target_doc: TargetDoc = bson::from_document(doc)?;

				if let Some(name) = target_doc.name {
					return Ok(name);
				}

				let mut names = Vec::new();
				for target_id in target_doc.targets {
					let name = self.fetch_handle_name_async(&target_id).await?;
					names.push(name);
				}

				Ok(match names.len() {
					0 => "".to_string(),
					1 => names[0].clone(),
					_ => format!("({})", names.join(" ")),
				})
			} else {
				Ok("".to_string())
			}
		})
	}
}
