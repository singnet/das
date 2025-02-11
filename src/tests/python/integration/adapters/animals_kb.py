from hyperon_das_atomdb.utils.expression_hasher import ExpressionHasher

inheritance_hash = ExpressionHasher.named_type_hash("Inheritance")
similarity_hash = ExpressionHasher.named_type_hash("Similarity")

human = ExpressionHasher.terminal_hash("Concept", "human")
monkey = ExpressionHasher.terminal_hash("Concept", "monkey")
chimp = ExpressionHasher.terminal_hash("Concept", "chimp")
mammal = ExpressionHasher.terminal_hash("Concept", "mammal")
reptile = ExpressionHasher.terminal_hash("Concept", "reptile")
snake = ExpressionHasher.terminal_hash("Concept", "snake")
dinosaur = ExpressionHasher.terminal_hash("Concept", "dinosaur")
triceratops = ExpressionHasher.terminal_hash("Concept", "triceratops")
earthworm = ExpressionHasher.terminal_hash("Concept", "earthworm")
rhino = ExpressionHasher.terminal_hash("Concept", "rhino")
vine = ExpressionHasher.terminal_hash("Concept", "vine")
ent = ExpressionHasher.terminal_hash("Concept", "ent")
animal = ExpressionHasher.terminal_hash("Concept", "animal")
plant = ExpressionHasher.terminal_hash("Concept", "plant")

node_docs = {}
node_docs[human] = {"type": "Concept", "name": "human"}
node_docs[monkey] = {"type": "Concept", "name": "monkey"}
node_docs[chimp] = {"type": "Concept", "name": "chimp"}
node_docs[mammal] = {"type": "Concept", "name": "mammal", "custom_attributes": {"name": "mammal"}}
node_docs[reptile] = {"type": "Concept", "name": "reptile"}
node_docs[snake] = {"type": "Concept", "name": "snake"}
node_docs[dinosaur] = {"type": "Concept", "name": "dinosaur"}
node_docs[triceratops] = {"type": "Concept", "name": "triceratops"}
node_docs[earthworm] = {"type": "Concept", "name": "earthworm"}
node_docs[rhino] = {"type": "Concept", "name": "rhino"}
node_docs[vine] = {"type": "Concept", "name": "vine"}
node_docs[ent] = {"type": "Concept", "name": "ent"}
node_docs[animal] = {"type": "Concept", "name": "animal"}
node_docs[plant] = {"type": "Concept", "name": "plant"}

inheritance_targets = [
    [human, mammal],
    [monkey, mammal],
    [chimp, mammal],
    [mammal, animal],
    [reptile, animal],
    [snake, reptile],
    [dinosaur, reptile],
    [triceratops, dinosaur],
    [earthworm, animal],
    [rhino, mammal],
    [vine, plant],
    [ent, plant],
]
inheritance = {}
inheritance_docs = {}
for source, target in inheritance_targets:
    if source in inheritance:
        row = inheritance[source]
    else:
        row = {}
        inheritance[source] = row
    row[target] = ExpressionHasher.expression_hash(inheritance_hash, [source, target])
    inheritance_docs[row[target]] = {
        "type": "Inheritance",
        "targets": [node_docs[source], node_docs[target]],
    }

similarity_targets = [
    [human, monkey],
    [human, chimp],
    [chimp, monkey],
    [snake, earthworm],
    [rhino, triceratops],
    [snake, vine],
    [human, ent],
    [monkey, human],
    [chimp, human],
    [monkey, chimp],
    [earthworm, snake],
    [triceratops, rhino],
    [vine, snake],
    [ent, human],
]
similarity = {}
similarity_docs = {}
for source, target in similarity_targets:
    if source in similarity:
        row = similarity[source]
    else:
        row = {}
        similarity[source] = row
    row[target] = ExpressionHasher.expression_hash(similarity_hash, [source, target])
    similarity_docs[row[target]] = {
        "type": "Similarity",
        "targets": [node_docs[source], node_docs[target]],
    }
