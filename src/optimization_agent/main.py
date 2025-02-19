import time
from hyperon_das import DistributedAtomSpace
from optimizer import QueryOptimizerIterator


def main():
    path = "/home/marcocapozzoli/Desktop/hub-potencializa/jobs/singularity-net/projects/das/src/optimization_agent/config.cfg"
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
            'VARIABLE', 'x',
            'VARIABLE', 'y'
    ]
    optimizer = QueryOptimizerIterator(config_file=path, query_tokens=query1)
    result = []
    for query_answer in optimizer:
        result.append(query_answer)

    # with open('./result_queries.txt', mode='+a') as f:
    #     f.write("\n")
    #     f.write("=== Final ===")
    #     f.write("\n")
    #     f.write("\n")
    #     for r in result:
    #         f.write(r.to_string())
    #         f.write("\n")

    # def populate_db():
    #     das = DistributedAtomSpace(
    #         atomdb='redis_mongo',
    #         mongo_hostname='localhost',
    #         mongo_port=27017,
    #         mongo_username='root',
    #         mongo_password='root',
    #         redis_hostname='localhost',
    #         redis_port=6379,
    #         redis_cluster=False,
    #         redis_ssl=False,
    #     )
    #     from hyperon_das_atomdb.database import LinkT, NodeT
    #     import random
    #     for i in range(100000):
    #         das.add_link(LinkT(
    #             type='Evaluation',
    #             targets=[
    #                 NodeT(type='Predicate', name='has_name:1'),
    #                 NodeT(type='Predicate', name='has_name:2'),
    #                 LinkT(
    #                     type='Set',
    #                     targets=[
    #                         NodeT(type='Reactome', name=f'R-HSA-{i}'),
    #                         LinkT(
    #                             type='Set',
    #                             targets=[
    #                                 NodeT(type='Reactome', name=f'R-HSA-{i}'),
    #                                 NodeT(type='Reactome', name=f'R-HSB-{i}'),
    #                                 NodeT(type='Concept', name='LTR-circle'),
    #                             ],
    #                         ),
    #                         NodeT(type='Concept', name='LTR-circle'),
    #                     ],
    #                 ),
    #             ],
    #             # custom_attributes={'truth_value': (0.88, 0.91)} # THE RIGHT WWAY
    #             custom_attributes={'strength': random.random(), 'confidence': random.random()}
    #         ))
    #         if i in [10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000]:
    #             print(f"comitando {i} atoms")
    #             das.commit_changes()
    #     das.commit_changes()
    #     return das

    # populate_db()


if __name__ == '__main__':
    main()