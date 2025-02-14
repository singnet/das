import time
from hyperon_das import DistributedAtomSpace
from optimizer import QueryOptimizerIterator


def main():
    path = "/home/marcocapozzoli/Desktop/hub-potencializa/jobs/singularity-net/projects/das/src/python/optimization_agent/config.cfg"
    query1 = [
        'LINK_TEMPLATE',
        'Evaluation', '2',
            'NODE', 'Predicate', 'Predicate:has_name',
            'VARIABLE', 'v2'
    ]
    query2 = [
        'OR', '2',
            'AND', '2',
                'LINK_TEMPLATE', 'Evaluation', '2',
                    'NODE', 'Predicate', 'Predicate:has_name',
                    'LINK_TEMPLATE', 'Set', '2',
                        'NODE', 'Reactome', 'Reactome:R-HSA-1',
                        'NODE', 'Concept', 'Concept:2-LTR circle formation',
                'LINK_TEMPLATE', 'Evaluation', '2',
                    'NODE', 'Predicate', 'Predicate:has_name',
                    'LINK_TEMPLATE', 'Set', '2',
                        'NODE', 'Reactome', 'Reactome:R-HSA-2',
                        'NODE', 'Concept', 'Concept:2-LTR circle formation',
            'LINK_TEMPLATE', 'Evaluation', '2',
                'NODE', 'Predicate', 'Predicate:has_name',
                'LINK_TEMPLATE', 'Set', '2',
                    'NODE', 'Reactome', 'Reactome:R-HSA-3',
                    'NODE', 'Concept', 'Concept:2-LTR circle formation'
    ]
    query3 = [
        'LINK_TEMPLATE',
        'Expression', '3',
            'NODE', 'Symbol', 'Similarity',
            'VARIABLE', 'v1',
            'VARIABLE', 'v2'
    ]
    optimizer = QueryOptimizerIterator(config_file=path, query_tokens=query3)

    # while not optimizer.is_empty():
    #     print(optimizer.pop())

    # time.sleep(30)
    
    # s = time.time()
    # um = next(optimizer)
    # print(f"Tempo para o primeiro next: {time.time() - s}")
    # s = time.time()
    # dois = next(optimizer)
    # print(f"Tempo para o segundo next: {time.time() - s}")
    # s = time.time()
    # tres = next(optimizer)
    # print(f"Tempo para o terceiro next: {time.time() - s}")

    for idx, op in enumerate(optimizer):
        print(f':: {idx} :: {optimizer.generation} ::')

    def populate_db():
        das = DistributedAtomSpace(
            atomdb='redis_mongo',
            mongo_hostname='localhost',
            mongo_port=27017,
            mongo_username='root',
            mongo_password='root',
            redis_hostname='localhost',
            redis_port=6379,
            redis_cluster=False,
            redis_ssl=False,
        )
        from hyperon_das_atomdb.database import LinkT, NodeT
        import random
        for i in range(10000):
            das.add_link(LinkT(
                type='Evaluation',
                targets=[
                    NodeT(type='Predicate', name='Predicate:has_name'),
                    LinkT(
                        type='Set',
                        targets=[
                            NodeT(type='Reactome', name=f'Reactome:R-HSA-{i}'),
                            NodeT(type='Concept', name='Concept:2-LTR circle formation'),
                        ],
                    ),
                ],
                # custom_attributes={'truth_value': (0.88, 0.91)} # THE RIGHT WWAY
                custom_attributes={'strength': random.random(), 'confidence': random.random()}
            ))
        # das.add_link(LinkT(
        #     type='Evaluation',
        #     targets=[
        #         NodeT(type='Predicate', name='Predicate:has_name'),
        #         LinkT(
        #             type='Set',
        #             targets=[
        #                 NodeT(type='Reactome', name='Reactome:R-HSA-2'),
        #                 NodeT(type='Concept', name='Concept:2-LTR circle formation'),
        #             ],
        #         ),
        #     ],
        #     # custom_attributes={'truth_value': (0.72, 0.99)}
        #     custom_attributes={'strength': 0.72, 'confidence': 0.99}
        # ))
        # das.add_link(LinkT(
        #     type='Evaluation',
        #     targets=[
        #         NodeT(type='Predicate', name='Predicate:has_name'),
        #         LinkT(
        #             type='Set',
        #             targets=[
        #                 NodeT(type='Reactome', name='Reactome:R-HSA-3'),
        #                 NodeT(type='Concept', name='Concept:2-LTR circle formation'),
        #             ],
        #         ),
        #     ],
        #     # custom_attributes={'truth_value': (0.43, 0.69)}
        #     custom_attributes={'strength': 0.43, 'confidence': 0.69}
        # ))
        das.commit_changes()
        return das

    # populate_db()


if __name__ == '__main__':
    main()