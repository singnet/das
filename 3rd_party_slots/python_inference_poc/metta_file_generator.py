import argparse
import random
from pathlib import Path
from random import randint
from typing import Any, Callable


class NodeLinkGenerator:
    debug = False

    def __init__(
        self,
        sentence_node_count: str,
        word_count: str,
        word_length: str,
        alphabet_range: str,
        debug: bool,
    ) -> None:
        self.sentence_node_count: int = int(sentence_node_count)
        self.word_node_count: int = 0
        self.word_count: int = int(word_count)
        self.word_length: int = int(word_length)
        self.alphabet_range: list[int] = [int(i) for i in alphabet_range.split('-')]
        self.link_word_count: int = 0
        self.link_letter_count: int = 0
        NodeLinkGenerator.debug = debug

    def generate_metta_file(self, writer: 'NodeLinkWriter') -> None:
        self.generate_sentence_structure(writer)


    def print_status(self):
        if NodeLinkGenerator.debug:
            print()
            print('Word Count:', self.word_count)
            print('Word Length:', self.word_length)
            print('Alphabet Range: ' + ' to '.join((chr(97 + k) for k in self.alphabet_range)))
            print('Sentence Nodes: ' + str(self.sentence_node_count))
            print('Word Nodes: ' + str(self.word_node_count))
            # print('Links Word: ' + str(self.link_word_count))
            # print('Links Letter: ' + str(self.link_letter_count))
            print()



    def generate_sentence_structure(self, writer: 'NodeLinkWriter') -> list[dict]:
        sentence_nodes = []
        words_nodes = []
        all_sentences = []

        for _ in range(self.sentence_node_count):
            word_list = [self._create_word() for _ in range(self.word_count)]
            sentence_name = '" "'.join(word_list)
            sentence_node = {'type': 'Sentence', 'name': sentence_name}
            # writer.add_node(sentence_node)
            sentence_nodes.append(sentence_node)
            sentence_link = {'type': 'Sentence', 'targets': [sentence_node]}
            writer.add_link(sentence_link)
            all_sentences.append(tuple([sentence_name, sentence_link]))

            for word in word_list:
                words_node = {'type': 'Word', 'name': word}
                writer.add_node(words_node)
                words_nodes.append(words_node)

            #     words_link = {'type': 'Word', 'targets': [words_node]}
                # writer.add_link(words_link)

                # contains_link = {'type': 'Contains', 'targets': [sentence_link, words_link]}
                # writer.add_link(contains_link)

        self.word_node_count = len(words_nodes)

                    
        writer.commit_changes()
        return sentence_nodes, words_nodes

    def generate_links_letter(self, node_list: list[dict[str, Any]]) -> dict[str, Any]:
        """
        Creates links of type Similarity between all pair of nodes which share at least one common
        letter in the same position in their names.
        Outputs a dict containing the link reference {node1->node2} as key and the number of
        common letters as value.
        Args:
            node_list (list[dict[str, Any]]): list of nodes

        Returns:
            letter_link_list (dict[str, Any]): dict containing the links as key and strength
                as value.

        """
        links_letter = {}
        total_links = 0
        for i, _ in enumerate(node_list):
            for j in range(i + 1, len(node_list)):
                if random.random() > self.letter_link_percentage:
                    continue
                key = f'{min(i, j)}->{max(i, j)}'
                strength = NodeLinkGenerator.compare_str(node_list[i]['name'], node_list[j]['name'])
                links_letter[key] = strength
                total_links += 1
        return links_letter

    def generate_links_word(self, node_list: list[dict[str, Any]]) -> dict[str, Any]:
        links_dict_word: dict[str, Any] = {}
        total_links = 0
        for i, v in enumerate(node_list):
            for j in range(i + 1, len(node_list)):
                if random.random() > self.word_link_percentage:
                    continue
                strength = self.compare_words(v['name'], node_list[j]['name'])
                links_dict_word[f'{i}->{j}'] = strength
                total_links += 1
        return links_dict_word

    @staticmethod
    def add_links(
        writer: 'NodeLinkWriter',
        links: dict[str, Any],
        node_list: list[dict[str, Any]],
        link_type: str,
        keyword_gen: Callable,
        strength_divisor: int = 1,
    ) -> None:
        """
        Adds link to an instance of Hyperon DAS.
        Args:
            writer (NodeLinkWriter): ...
            links (dict[str, Any]): dict containing links to add
            node_list (list[dict[str, Any]]): list of Nodes to retrieve as link's targets
            link_type (str): Type of link
            keyword_gen (Callble): function to create a keyword
            strength_divisor (int): Divisor number to divide the strength of the link

        Returns:
            None

        """
        for k, v in links.items():
            targets = [node_list[int(i)] for i in k.split('->')]
            keyword = keyword_gen()
            link = {
                'type': link_type,
                'targets': targets,
                'strength': v / strength_divisor,
                'keyword': keyword,
                'indexed_keyword': keyword,
            }
            writer.add_link(link)
        writer.commit_changes()

    def _create_word(self) -> str:
        return ''.join([chr(97 + randint(*self.alphabet_range)) for _ in range(self.word_length)])

class WriterBuffer:
    def __init__(self):
        self.output_buffer = []

    def write(self, data: str) -> None:
        data = data.strip()
        if not data:
            return
        if ':' in data:
            return
        self.output_buffer.extend(data.split('\n'))


class NodeLinkWriter:

    base_path = Path(__file__).resolve().parent

    def __init__(self, filename, output_buffer: WriterBuffer = None) -> None:
        self.buffer_nodes = []
        self.buffer_links = []
        self.types = set()
        self.filename = self.base_path / filename if filename else None
        self.output_buffer = output_buffer

    def add_node(self, node: dict) -> None:
        self.types.add(node.get('type'))
        self.buffer_nodes.append(node)

    def add_link(self, link: dict) -> None:
        self.types.add(link.get('type'))
        self.buffer_links.append(link)

    def commit_changes(self) -> None:
        def write_expression(f, links) -> str:
            result = ""
            for link in links:
                expression = f"({link['type']}"
                for target in link['targets']:
                    if 'name' in target:
                        expression += f' "{target["name"]}"'
                    else:
                        expression += " " + write_expression(None, [target])
                if 'tv' in link:
                    expression += " (tv " + str(link['tv'][0]) + str(link['tv'][1]) + ")"
                expression += ")"
                result += expression + "\n"
            if f is not None:
                f.write(result)
                f.write('\n')

            return result.strip()
        if self.filename:
            with open(self.filename, 'a+', encoding='utf8') as f:
                for t in self.types:
                    f.write(f'(: {t} Type)\n')
                for v in self.buffer_nodes:
                    f.write(f"(: \"{v['name']}\" {v['type']})\n")

                write_expression(f, self.buffer_links)
        if self.output_buffer:
            # for t in self.types:
            #     self.output_buffer.write(f'(: {t} Type)\n')
            # for v in self.buffer_nodes:
            #     self.output_buffer.write(f"(: \"{v['name']}\" {v['type']})\n")

            write_expression(self.output_buffer, self.buffer_links)

        self.types.clear()
        self.buffer_nodes.clear()
        self.buffer_links.clear()


def main():
    parser = argparse.ArgumentParser(description='Create MeTTa file.')

    parser.add_argument("--filename", default='output.metta', help="Filename")
    parser.add_argument(
        "--sentence-node-count",
        default=3,
        help="Number of Sentence nodes to be generated, eg: 100",
    )
    parser.add_argument("--word-count", default=3, help="How many words make up the node, eg: 10")
    parser.add_argument("--word-length", default=3, help="Number of characters in the world, eg: 5")
    parser.add_argument("--alphabet-range", default="0-3", help="Alphabet range, eg: 2-5")

    args = parser.parse_args()

    generator = NodeLinkGenerator(
        int(args.sentence_node_count),
        int(args.word_count),
        int(args.word_length),
        args.alphabet_range,
        debug=True,
    )
    generator.generate_metta_file(writer=NodeLinkWriter(args.filename))
    generator.print_status()


if __name__ == '__main__':
    main()