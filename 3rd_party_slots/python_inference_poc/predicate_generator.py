import argparse
import random
import re
import time
import json
from metta_file_generator import NodeLinkGenerator, NodeLinkWriter, WriterBuffer


def read_sentences(file_path):
    with open(file_path, 'r') as file:
        sentences = file.readlines()
    return [sentence.strip() for sentence in sentences if sentence.strip() and sentence.startswith('(Sentence')]

def parse_config_file(config_file, args):
    config = {}
    with open(config_file, 'r') as file:
        config = json.load(file)
        config_copy = {}
        for k,v in config.items():
            if isinstance(v, dict):
                for k2, v2 in v.items():
                    config_copy[f"{k}-{k2}"] = v2
            else:
                config_copy[k] = v
        config = config_copy
    for key, value in config.items():
        key = key.replace('-', '_') 
        if hasattr(args, key):
            setattr(args, key, value)
        else:
            print(f"Warning: Config key '{key}' does not match any argument in the script. Skipping.")
    return args

def letter_predicate(sentences, token=None, letter=None, letter_percent=0.6, randomness=0.8, **kwargs):
    if kwargs:
        token = kwargs.get('letter_predicate_token', token)
        letter = kwargs.get('letter_predicate_letter', letter)
        letter_percent = kwargs.get('letter_predicate_letter_percent', letter_percent)
        randomness = kwargs.get('letter_predicate_randomness', randomness)
    print(f"### Running letter predicate with token: {token}, letter: {letter}, letter_percent: {letter_percent}, randomness: {randomness}, sentences: {len(sentences)}")
    predicates = []
    for sentence in sentences:
        if time.time() % 2 == 0:  # Simulate some processing delay
            print(f"Processing sentence: {sentence}")
        parsed_sentence = ''.join(sentence.replace(' ', '').split('"')[1:-1])
        count_letter = parsed_sentence.count(letter)
        if count_letter > 0 and count_letter / len(parsed_sentence) >= letter_percent:
            if random.random() > randomness:
                predicates.append(f'(EVALUATION (PREDICATE "{token}") (CONCEPT {sentence}))')
    return predicates

def wildcard_predicate(sentences, token=None, wildcards=None, randomness=0.8, **kwargs):
    if kwargs:
        token = kwargs.get('wildcard_predicate_token', token)
        wildcards = kwargs.get('wildcard_predicate_wildcards', wildcards)
        randomness = kwargs.get('wildcard_predicate_randomness', randomness)
    print(f"### Running wildcard predicate with token: {token}, wildcards: {wildcards}, randomness: {randomness}")
    predicates = []
    for sentence in sentences:
        split_sentence = sentence[:-1].replace('"', '').split(' ')[1:]
        for s in split_sentence:
            for wildcard in wildcards:
                if re.match(wildcard.replace('*', '.'), s):
                    if random.random() > randomness:
                        predicates.append(f'(EVALUATION (PREDICATE "{token}") (CONCEPT {sentence}))')
                    break
    return predicates

def start_end_predicate(sentences, token=None, start_letter=None, end_letter=None, randomness=0.8, **kwargs):
    if kwargs:
        token = kwargs.get('start_end_predicate_token', token)
        start_letter = kwargs.get('start_end_predicate_start_letter', start_letter)
        end_letter = kwargs.get('start_end_predicate_end_letter', end_letter)
        randomness = kwargs.get('start_end_predicate_randomness', randomness)
    print(f"### Running start-end predicate with token: {token}, start_letter: {start_letter}, end_letter: {end_letter}, randomness: {randomness}")
    predicates = []
    for sentence in sentences:
        split_sentence = sentence[:-1].replace('"', '').split(' ')[1:]
        for s in split_sentence:
            if s.startswith(start_letter) and s.endswith(end_letter):
                if random.random() > randomness:
                    predicates.append(f'(EVALUATION (PREDICATE "{token}") (CONCEPT {sentence}))')
                    break
    return predicates

def low_diversity_predicate(sentences, token=None, letters=None, randomness=0.8, **kwargs):
    if kwargs:
        token = kwargs.get('low_diversity_predicate_token', token)
        letters = kwargs.get('low_diversity_predicate_letters', letters)
        randomness = kwargs.get('low_diversity_predicate_randomness', randomness)
    print(f"### Running low-diversity predicate with token: {token}, letters: {letters}, randomness: {randomness}")
    predicates = []
    for sentence in sentences:
        parsed_sentence = ''.join(sentence.replace(' ', '').split('"')[1:-1])
        letter_count = 0
        for letter in letters:
            if parsed_sentence.count(letter) > 0:
                letter_count += 1
        if letter_count <= len(letters) / 2 and random.random() > randomness:
            predicates.append(f'(EVALUATION (PREDICATE "{token}") (CONCEPT {sentence}))')

    return predicates

def create_predicates(args, predicate_func, predicate_count):
    predicates = []
    generator = NodeLinkGenerator(
        int(predicate_count),
        int(args.word_count),
        int(args.word_length),
        args.alphabet_range,
        debug=True,
    )
    writer_buffer = WriterBuffer()
    args_cp = vars(args)
    for k, v in args_cp.items():
        if 'randomness' in k:
            args_cp[k] = 0
    count = 0
    while len(predicates) < predicate_count:
        generator.generate_metta_file(writer=NodeLinkWriter(None, writer_buffer))
        for p in predicate_func(writer_buffer.output_buffer, **vars(args)):
            predicates.append(p)
        count += len(predicates)

    return predicates[:predicate_count]  # Return only the requested number of predicates

def write_predicates(filename, biased_predicates, write_words_and_sentences=True, append=False):
    sbuffer = set()
    wbuffer = set()
    with open(filename, 'w' if not append else 'a') as f:
        if write_words_and_sentences:
            for predicate in biased_predicates:
                sentence  = '(' + predicate.split('(')[-1][:-2]
                words = sentence[:-1].replace('"', '').split(' ')[1:]
                for word in words:
                    wbuffer.add(f'(: "{word}" Word)\n')
                sbuffer.add(sentence + '\n')
            for w in wbuffer:
                f.write(w)
            for b in sbuffer:
                f.write(b)
        for predicate in biased_predicates:
            f.write(predicate + '\n')


def generate_biased_predicates(args, predicate_lengths):
    print(f"### Generating biased predicates based on the provided arguments, predicates: {args.bias_predicates}, strength: {args.bias_strength}")
    biased_predicates = []

    for predicate in args.bias_predicates:
        if predicate in predicate_lengths:
            bias_count = int(predicate_lengths[predicate] * args.bias_strength)
            if predicate == 'letter-predicate':
                biased_predicates.extend(create_predicates(args, letter_predicate, bias_count))
            elif predicate == 'wildcard-predicate':
                biased_predicates.extend(create_predicates(args, wildcard_predicate, bias_count))
            elif predicate == 'start-end-predicate':
                biased_predicates.extend(create_predicates(args, start_end_predicate, bias_count))
            elif predicate == 'low-diversity-predicate':
                biased_predicates.extend(create_predicates(args, low_diversity_predicate, bias_count))
        else:
            print(f"Warning: Predicate '{predicate}' not found in predicate lengths. Skipping bias generation for this predicate.")
            continue
    write_predicates(args.bias_filename, biased_predicates)
    return biased_predicates

def main():
    parser = argparse.ArgumentParser(description="Process some sentences.")
    parser.add_argument("file", type=str, help="The file to process")
    parser.add_argument("--seed", type=int, default=42, help="Random seed for reproducibility")
    parser.add_argument("--config-file", type=str, default=None, help="Path to the configuration file")
    parser.add_argument("--letter-predicate-token", type=str, default='feature_a', help="Token for letter predicate")
    parser.add_argument("--letter-predicate-letter", type=str, default='a', help="Letter for letter predicate")
    parser.add_argument("--letter-predicate-percent", type=float, default=0.6, help="Percentage of letters for letter predicate")
    parser.add_argument("--letter-predicate-letter-percent", type=float, default=0.6, help="Percentage of letters for letter predicate")
    parser.add_argument("--letter-predicate-randomness", type=float, default=0.8, help="Randomness for letter predicate")
    parser.add_argument("--wildcard-predicate-token", type=str, default='feature_b', help="Token for wildcard predicate")
    parser.add_argument("--wildcard-predicate-wildcards", type=str, nargs='+', default=['*ac', 'cb*'], help="Wildcards for wildcard predicate")
    parser.add_argument("--wildcard-predicate-randomness", type=float, default=0.8, help="Randomness for wildcard predicate")
    parser.add_argument("--start-end-predicate-token", type=str, default='feature_c', help="Token for start-end predicate")
    parser.add_argument("--start-end-predicate-start-letter", type=str, default='c', help="Start letter for start-end predicate")
    parser.add_argument("--start-end-predicate-end-letter", type=str, default='d', help="End letter for start-end predicate")
    parser.add_argument("--start-end-predicate-randomness", type=float, default=0.8, help="Randomness for start-end predicate")
    parser.add_argument("--low-diversity-predicate-token", type=str, default='feature_d', help="Token for low-diversity predicate")
    parser.add_argument("--low-diversity-predicate-letters", type=str, nargs='+', default=['a', 'b', 'c', 'd', 'e'], help="Letters for low-diversity predicate")
    parser.add_argument("--low-diversity-predicate-randomness", type=float, default=0.8, help="Randomness for low-diversity predicate")
    parser.add_argument("--bias-predicates", type=str, nargs='+', default=['letter-predicate', 'low-diversity-predicate'], help="Predicates to bias")
    parser.add_argument("--bias-strength", type=float, default=0.5, help="Strength of the bias")
    parser.add_argument("--bias-filename", type=str, default='bias.metta', help="Filename for biased predicates")
    parser.add_argument("--metta-filename", type=str, default='ooo.metta', help="Output MeTTa filename")
    parser.add_argument("--sentence-node-count", type=int, default=3, help="Number of Sentence nodes to be generated")
    parser.add_argument("--word-count", type=int, default=3, help="How many words make up the node")
    parser.add_argument("--word-length", type=int, default=3, help="Number of characters in the word")
    parser.add_argument("--alphabet-range", type=str, default="0-3", help="Alphabet range, eg: 2-5")
    args = parser.parse_args()
    if args.config_file:
        args = parse_config_file(args.config_file, args)
    # initialize seed
    random.seed(args.seed)
    total_time = time.time()
    predicates = []
    # Read sentences from the file
    read_sentences_time = time.time()
    sentences = read_sentences(args.file)
    read_sentences_duration = time.time() - read_sentences_time
    # Create predicates based on the sentences
    letter_predicates_time = time.time()
    letter_predicates = letter_predicate(sentences, args.letter_predicate_token, args.letter_predicate_letter, args.letter_predicate_letter_percent, args.letter_predicate_randomness)
    letter_predicates_duration = time.time() - letter_predicates_time
    predicates.extend(letter_predicates)
    # Wildcard predicates
    wildcard_predicates_time = time.time()
    wildcard_predicates = wildcard_predicate(sentences, args.wildcard_predicate_token, args.wildcard_predicate_wildcards, args.wildcard_predicate_randomness)
    wildcard_predicates_duration = time.time() - wildcard_predicates_time
    predicates.extend(wildcard_predicates)
    # Start-end predicates
    start_end_predicates_time = time.time()
    start_end_predicates = start_end_predicate(sentences, args.start_end_predicate_token, args.start_end_predicate_start_letter, args.start_end_predicate_end_letter, args.start_end_predicate_randomness)
    start_end_predicates_duration = time.time() - start_end_predicates_time
    predicates.extend(start_end_predicates)
    # Low-diversity predicates
    low_diversity_predicates_time = time.time()
    low_diversity_predicates = low_diversity_predicate(sentences, args.low_diversity_predicate_token, args.low_diversity_predicate_letters, args.low_diversity_predicate_randomness)
    low_diversity_predicates_duration = time.time() - low_diversity_predicates_time
    predicates.extend(low_diversity_predicates)
    total_time = time.time() - total_time

    biased_predicates_time = time.time()
    if args.bias_strength > 0:
        predicate_lengths = {
            'letter-predicate': len(letter_predicates),
            'wildcard-predicate': len(wildcard_predicates),
            'start-end-predicate': len(start_end_predicates),
            'low-diversity-predicate': len(low_diversity_predicates)
        }
        biased_predicates = generate_biased_predicates(args, predicate_lengths)
        # predicates.extend(biased_predicates)
    biased_predicates_duration = time.time() - biased_predicates_time

    write_predicates(args.metta_filename, predicates, False, True)

    # Report
    print("######################### Summary #########################")
    print(f"Generated {len(predicates)} predicates based on the sentences in {args.file}.")
    print(f"Letter predicates: {len(letter_predicates)}")
    print(f"Wildcard predicates: {len(wildcard_predicates)}")
    print(f"Start-end predicates: {len(start_end_predicates)}")
    print(f"Low-diversity predicates: {len(low_diversity_predicates)}")
    print(f"Biased predicates: {len(biased_predicates) if args.bias_strength > 0 else 0}")
    print("######################### Duration #########################")
    print(f"Read sentences duration: {read_sentences_duration:.2f} seconds")
    print(f"Letter predicates duration: {letter_predicates_duration:.2f} seconds")
    print(f"Wildcard predicates duration: {wildcard_predicates_duration:.2f} seconds")
    print(f"Start-end predicates duration: {start_end_predicates_duration:.2f} seconds")
    print(f"Low-diversity predicates duration: {low_diversity_predicates_duration:.2f} seconds")
    print(f"Biased predicates duration: {biased_predicates_duration:.2f} seconds")
    print(f"Total time: {total_time:.2f} seconds")

main()