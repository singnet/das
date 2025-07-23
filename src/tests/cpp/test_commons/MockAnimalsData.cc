#include "MockAnimalsData.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace atomdb;

void load_animals_data() {
    // Drop Redis and MongoDB databases
    LOG_INFO("Dropping Redis and MongoDB databases...");
    auto db = dynamic_pointer_cast<RedisMongoDB>(AtomDBSingleton::get_instance());
    db->drop_all();

    string tokens = "LINK_TEMPLATE Expression 3 NODE Symbol Similarity VARIABLE v1 VARIABLE v2";
    vector<vector<string>> index_entries = {{"_", "*", "*"}, {"_", "v1", "*"}, {"_", "*", "v2"}};
    LOG_INFO("Adding pattern index schema for: " + tokens + "...");
    db->add_pattern_index_schema(tokens, index_entries);

    tokens = "LINK_TEMPLATE Expression 3 NODE Symbol Inheritance VARIABLE v1 VARIABLE v2";
    index_entries = {{"_", "*", "*"}, {"_", "v1", "*"}, {"_", "*", "v2"}};
    LOG_INFO("Adding pattern index schema for: " + tokens + "...");
    db->add_pattern_index_schema(tokens, index_entries);

    tokens = "LINK_TEMPLATE Expression 2 NODE Symbol OddLink VARIABLE v1";
    index_entries = {{"_", "*"}, {"_", "v1"}};
    LOG_INFO("Adding pattern index schema for: " + tokens + "...");
    db->add_pattern_index_schema(tokens, index_entries);

    // Nodes
    vector<Node*> nodes;

    // Common Symbols
    auto type = new Node("Symbol", "Type");
    auto concept = new Node("Symbol", "Concept");
    auto colon = new Node("Symbol", ":");
    auto expression = new Node("Symbol", "Expression");
    auto metta_type = new Node("Symbol", "MettaType");
    auto typedef_subtype_mark = new Node("Symbol", "<:");
    auto arrow = new Node("Symbol", "->");

    nodes.push_back(type);
    nodes.push_back(concept);
    nodes.push_back(colon);
    nodes.push_back(expression);
    nodes.push_back(metta_type);
    nodes.push_back(typedef_subtype_mark);
    nodes.push_back(arrow);

    auto similarity = new Node("Symbol", "Similarity");
    auto inheritance = new Node("Symbol", "Inheritance");
    auto oddlink = new Node("Symbol", "OddLink");

    nodes.push_back(similarity);
    nodes.push_back(inheritance);
    nodes.push_back(oddlink);

    auto human = new Node("Symbol", "\"human\"");
    auto monkey = new Node("Symbol", "\"monkey\"");
    auto chimp = new Node("Symbol", "\"chimp\"");
    auto snake = new Node("Symbol", "\"snake\"");
    auto earthworm = new Node("Symbol", "\"earthworm\"");
    auto rhino = new Node("Symbol", "\"rhino\"");
    auto triceratops = new Node("Symbol", "\"triceratops\"");
    auto vine = new Node("Symbol", "\"vine\"");
    auto ent = new Node("Symbol", "\"ent\"");
    auto mammal = new Node("Symbol", "\"mammal\"");
    auto animal = new Node("Symbol", "\"animal\"");
    auto reptile = new Node("Symbol", "\"reptile\"");
    auto dinosaur = new Node("Symbol", "\"dinosaur\"");
    auto plant = new Node("Symbol", "\"plant\"");

    nodes.push_back(human);
    nodes.push_back(monkey);
    nodes.push_back(chimp);
    nodes.push_back(snake);
    nodes.push_back(earthworm);
    nodes.push_back(rhino);
    nodes.push_back(triceratops);
    nodes.push_back(vine);
    nodes.push_back(ent);
    nodes.push_back(mammal);
    nodes.push_back(animal);
    nodes.push_back(reptile);
    nodes.push_back(dinosaur);
    nodes.push_back(plant);

    LOG_INFO("Adding nodes to db...");
    db->add_nodes(nodes);
    nodes.clear();

    // Links
    vector<Link*> links;

    // Types
    auto similarity_type = new Link(
        "Expression", {colon->handle(), similarity->handle(), type->handle()});   // (: Similarity Type)
    auto concept_type = new Link(
        "Expression", {colon->handle(), concept->handle(), type->handle()});      // (: Concept Type)
    auto inheritance_type = new Link(
        "Expression", {colon->handle(), inheritance->handle(), type->handle()});  // (: Inheritance Type)

    links.push_back(similarity_type);
    links.push_back(concept_type);
    links.push_back(inheritance_type);

    LOG_INFO("Adding Type links to db...");
    db->add_links(links);
    links.clear();

    // Concepts
    auto human_concept = new Link(
        "Expression", {colon->handle(), human->handle(), concept->handle()});   // (: "human" Concept)
    auto monkey_concept = new Link(
        "Expression", {colon->handle(), monkey->handle(), concept->handle()});  // (: "monkey" Concept)
    auto chimp_concept = new Link(
        "Expression", {colon->handle(), chimp->handle(), concept->handle()});   // (: "chimp" Concept)
    auto snake_concept = new Link(
        "Expression", {colon->handle(), snake->handle(), concept->handle()});   // (: "snake" Concept)
    auto earthworm_concept =
        new Link("Expression",
                 {colon->handle(), earthworm->handle(), concept->handle()});   // (: "earthworm" Concept)
    auto rhino_concept = new Link(
        "Expression", {colon->handle(), rhino->handle(), concept->handle()});  // (: "rhino" Concept)
    auto triceratops_concept = new Link(
        "Expression",
        {colon->handle(), triceratops->handle(), concept->handle()});  // (: "triceratops" Concept)
    auto vine_concept = new Link(
        "Expression", {colon->handle(), vine->handle(), concept->handle()});     // (: "vine" Concept)
    auto ent_concept = new Link(
        "Expression", {colon->handle(), ent->handle(), concept->handle()});      // (: "ent" Concept)
    auto mammal_concept = new Link(
        "Expression", {colon->handle(), mammal->handle(), concept->handle()});   // (: "mammal" Concept)
    auto animal_concept = new Link(
        "Expression", {colon->handle(), animal->handle(), concept->handle()});   // (: "animal" Concept)
    auto reptile_concept = new Link(
        "Expression", {colon->handle(), reptile->handle(), concept->handle()});  // (: "reptile" Concept)
    auto dinosaur_concept =
        new Link("Expression",
                 {colon->handle(), dinosaur->handle(), concept->handle()});    // (: "dinosaur" Concept)
    auto plant_concept = new Link(
        "Expression", {colon->handle(), plant->handle(), concept->handle()});  // (: "plant" Concept)

    links.push_back(human_concept);
    links.push_back(monkey_concept);
    links.push_back(chimp_concept);
    links.push_back(snake_concept);
    links.push_back(earthworm_concept);
    links.push_back(rhino_concept);
    links.push_back(triceratops_concept);
    links.push_back(vine_concept);
    links.push_back(ent_concept);
    links.push_back(mammal_concept);
    links.push_back(animal_concept);
    links.push_back(reptile_concept);
    links.push_back(dinosaur_concept);
    links.push_back(plant_concept);

    LOG_INFO("Adding Concept links to db...");
    db->add_links(links);
    links.clear();

    // Similarity
    auto similarity_human_monkey = new Link(
        "Expression",
        {similarity->handle(), human->handle(), monkey->handle()});  // (Similarity "human" "monkey")
    auto similarity_human_chimp = new Link(
        "Expression",
        {similarity->handle(), human->handle(), chimp->handle()});  // (Similarity "human" "chimp")
    auto similarity_chimp_monkey = new Link(
        "Expression",
        {similarity->handle(), chimp->handle(), monkey->handle()});  // (Similarity "chimp" "monkey")
    auto similarity_snake_earthworm =
        new Link("Expression",
                 {similarity->handle(),
                  snake->handle(),
                  earthworm->handle()});  // (Similarity "snake" "earthworm")
    auto similarity_rhino_triceratops =
        new Link("Expression",
                 {similarity->handle(),
                  rhino->handle(),
                  triceratops->handle()});  // (Similarity "rhino" "triceratops")
    auto similarity_snake_vine = new Link(
        "Expression",
        {similarity->handle(), snake->handle(), vine->handle()});  // (Similarity "snake" "vine")
    auto similarity_human_ent =
        new Link("Expression",
                 {similarity->handle(), human->handle(), ent->handle()});  // (Similarity "human" "ent")
    auto similarity_monkey_human = new Link(
        "Expression",
        {similarity->handle(), monkey->handle(), human->handle()});  // (Similarity "monkey" "human")
    auto similarity_chimp_human = new Link(
        "Expression",
        {similarity->handle(), chimp->handle(), human->handle()});  // (Similarity "chimp" "human")
    auto similarity_monkey_chimp = new Link(
        "Expression",
        {similarity->handle(), monkey->handle(), chimp->handle()});  // (Similarity "monkey" "chimp")
    auto similarity_earthworm_snake = new Link("Expression",
                                               {similarity->handle(),
                                                earthworm->handle(),
                                                snake->handle()});  // (Similarity "earthworm" "snake")
    auto similarity_triceratops_rhino =
        new Link("Expression",
                 {similarity->handle(),
                  triceratops->handle(),
                  rhino->handle()});  // (Similarity "triceratops" "rhino")
    auto similarity_vine_snake = new Link(
        "Expression",
        {similarity->handle(), vine->handle(), snake->handle()});  // (Similarity "vine" "snake")
    auto similarity_ent_human =
        new Link("Expression",
                 {similarity->handle(), ent->handle(), human->handle()});  // (Similarity "ent" "human")

    links.push_back(similarity_human_monkey);
    links.push_back(similarity_human_chimp);
    links.push_back(similarity_chimp_monkey);
    links.push_back(similarity_snake_earthworm);
    links.push_back(similarity_rhino_triceratops);
    links.push_back(similarity_snake_vine);
    links.push_back(similarity_human_ent);
    links.push_back(similarity_monkey_human);
    links.push_back(similarity_chimp_human);
    links.push_back(similarity_monkey_chimp);
    links.push_back(similarity_earthworm_snake);
    links.push_back(similarity_triceratops_rhino);
    links.push_back(similarity_vine_snake);
    links.push_back(similarity_ent_human);

    LOG_INFO("Adding similarity links to db...");
    db->add_links(links);
    links.clear();

    // Inheritance
    auto inheritance_human_mammal = new Link(
        "Expression",
        {inheritance->handle(), human->handle(), mammal->handle()});  // (Inheritance "human" "mammal")
    auto inheritance_monkey_mammal = new Link(
        "Expression",
        {inheritance->handle(), monkey->handle(), mammal->handle()});  // (Inheritance "monkey" "mammal")
    auto inheritance_chimp_mammal = new Link(
        "Expression",
        {inheritance->handle(), chimp->handle(), mammal->handle()});  // (Inheritance "chimp" "mammal")
    auto inheritance_mammal_animal = new Link(
        "Expression",
        {inheritance->handle(), mammal->handle(), animal->handle()});  // (Inheritance "mammal" "animal")
    auto inheritance_reptile_animal = new Link("Expression",
                                               {inheritance->handle(),
                                                reptile->handle(),
                                                animal->handle()});  // (Inheritance "reptile" "animal")
    auto inheritance_snake_reptile = new Link(
        "Expression",
        {inheritance->handle(), snake->handle(), reptile->handle()});  // (Inheritance "snake" "reptile")
    auto inheritance_dinosaur_reptile =
        new Link("Expression",
                 {inheritance->handle(),
                  dinosaur->handle(),
                  reptile->handle()});  // (Inheritance "dinosaur" "reptile")
    auto inheritance_triceratops_dinosaur =
        new Link("Expression",
                 {inheritance->handle(),
                  triceratops->handle(),
                  dinosaur->handle()});  // (Inheritance "triceratops" "dinosaur")
    auto inheritance_earthworm_animal =
        new Link("Expression",
                 {inheritance->handle(),
                  earthworm->handle(),
                  animal->handle()});  // (Inheritance "earthworm" "animal")
    auto inheritance_rhino_mammal = new Link(
        "Expression",
        {inheritance->handle(), rhino->handle(), mammal->handle()});  // (Inheritance "rhino" "mammal")
    auto inheritance_vine_plant = new Link(
        "Expression",
        {inheritance->handle(), vine->handle(), plant->handle()});  // (Inheritance "vine" "plant")
    auto inheritance_ent_plant = new Link(
        "Expression",
        {inheritance->handle(), ent->handle(), plant->handle()});  // (Inheritance "ent" "plant")

    links.push_back(inheritance_human_mammal);
    links.push_back(inheritance_monkey_mammal);
    links.push_back(inheritance_chimp_mammal);
    links.push_back(inheritance_mammal_animal);
    links.push_back(inheritance_reptile_animal);
    links.push_back(inheritance_snake_reptile);
    links.push_back(inheritance_dinosaur_reptile);
    links.push_back(inheritance_triceratops_dinosaur);
    links.push_back(inheritance_earthworm_animal);
    links.push_back(inheritance_rhino_mammal);
    links.push_back(inheritance_vine_plant);
    links.push_back(inheritance_ent_plant);

    LOG_INFO("Adding inheritance links to db...");
    db->add_links(links);
    links.clear();

    // OddLinks
    auto oddlink_earthworm_snake =
        new Link("Expression",
                 {oddlink->handle(),
                  similarity_earthworm_snake->handle()});  // (OddLink (Similarity "earthworm" "snake"))
    auto oddlink_triceratops_rhino = new Link(
        "Expression",
        {oddlink->handle(),
         similarity_triceratops_rhino->handle()});  // (OddLink (Similarity "triceratops" "rhino"))
    auto oddlink_vine_snake = new Link(
        "Expression",
        {oddlink->handle(), similarity_vine_snake->handle()});  // (OddLink (Similarity "vine" "snake"))
    auto oddlink_ent_human = new Link(
        "Expression",
        {oddlink->handle(), similarity_ent_human->handle()});  // (OddLink (Similarity "ent" "human"))
    auto oddlink_snake_earthworm =
        new Link("Expression",
                 {oddlink->handle(),
                  similarity_snake_earthworm->handle()});  // (OddLink (Similarity "snake" "earthworm"))
    auto oddlink_rhino_triceratops = new Link(
        "Expression",
        {oddlink->handle(),
         similarity_rhino_triceratops->handle()});  // (OddLink (Similarity "rhino" "triceratops"))
    auto oddlink_snake_vine = new Link(
        "Expression",
        {oddlink->handle(), similarity_snake_vine->handle()});  // (OddLink (Similarity "snake" "vine"))
    auto oddlink_human_ent = new Link(
        "Expression",
        {oddlink->handle(), similarity_human_ent->handle()});  // (OddLink (Similarity "human" "ent"))
    auto oddlink_ent_plant = new Link(
        "Expression",
        {oddlink->handle(), inheritance_ent_plant->handle()});  // (OddLink (Inheritance "ent" "plant"))

    links.push_back(oddlink_earthworm_snake);
    links.push_back(oddlink_triceratops_rhino);
    links.push_back(oddlink_vine_snake);
    links.push_back(oddlink_ent_human);
    links.push_back(oddlink_snake_earthworm);
    links.push_back(oddlink_rhino_triceratops);
    links.push_back(oddlink_snake_vine);
    links.push_back(oddlink_human_ent);
    links.push_back(oddlink_ent_plant);

    LOG_INFO("Adding odd links to db...");
    db->add_links(links);
}
