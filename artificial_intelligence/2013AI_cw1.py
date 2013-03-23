# This is 2013AI_cw1.py by Pengyu CHEN(cpy.prefers.you@gmail.com)
# As the course work of Artificial Intelligence, 2013.02-2013.04
# COPYLEFT, ALL WRONGS RESERVED.

# Original problem:
#   Find the maximum of f(x) = 10 + sin(1 / x) / ((x - 0.16) ** 2 + 0.1)
#   on the range (0, 1) with tolarence 1E-6

import math
import random

# the original f(x)
def f(x):
    return 10 + math.sin(1 / x) / ((x - 0.16) ** 2 + 0.1)

# map a gene to a floating point number
def gene2f(gene, gene_size):
    return (gene + 1) / (2 ** gene_size)    # hardcoded mapping for range (0,1) 

# evaluate gene
def eval_gene(gene, gene_size):
    x = gene2f(gene, gene_size)
    return f(x)

# generate a randomized gene
def random_gene(gene_size):
    return random.getrandbits(gene_size)

# get a hybrid from two parent genes with a randomized bit mask
def hybridize(parent_gene, gene_size):
    mask = random.getrandbits(gene_size)
    p0 = parent_gene[0] & mask
    p1 = parent_gene[1] & ~mask
    return p0 | p1

# get a mutation from one parent gene with a randomized bit change
def mutate(parent_gene, gene_size):
    mutation_point = random.randrange(gene_size - 1)
    return parent_gene ^ (1 << mutation_point)

def main():
    # parameters and initializations
    tolerance = 1E-6
    gene_size = math.ceil(math.log(1 / tolerance, 2))    # hardcoded mapping for range (0,1)
    population_size = 100
    multiply_size = population_size * 7
    inher_ratio = 0.7
    skip_ratio = 0.1    # top individuals that skips selection and move to new
                        # generation directly
    population = [(gene, eval_gene(gene, gene_size)) for gene in 
        (random_gene(gene_size) for i in range(population_size))]
    print("Initialization done.")
    # the main loop
    iter_count = 32
    report_interval = 1
    for _i in range(iter_count):
        new_generation = []
        # inheritance
        for i in range(int(multiply_size * inher_ratio)):
            parent_gene = [random.choice(population)[0] for i in range(2)]
            child_gene = hybridize(parent_gene, gene_size)
            new_generation.append((child_gene, eval_gene(child_gene, gene_size)))
        # mutation
        for i in range(multiply_size - len(new_generation)):
            parent_gene = random.choice(population)[0]
            child_gene = mutate(parent_gene, gene_size)
            new_generation.append((child_gene, eval_gene(child_gene, gene_size)))
        population += new_generation
        # selection
        eval_sum = sum(gene_eval for (gene, gene_eval) in population)
        selector = sorted([random.random() for i in 
            range(int(population_size * (1 - skip_ratio)))])
        selected = []
        cur_eval_sum = 0
        for gene, gene_eval in population:
            cur_eval_sum += gene_eval
            sel = cur_eval_sum / eval_sum
            while selector and sel > selector[0]:
                selected += [(gene, gene_eval)]
                selector = selector[1:]
        # move top individuals directly to the next generation
        population = sorted(population, key = lambda x: -x[1])
        skip_len = population_size - len(selected)
        selected += population[:skip_len]
        population = selected
        # progress report
        if ((_i + 1) % report_interval == 0):
            gene, gene_eval = max(population, key = lambda x: x[1])
            print("loop %d/%d: f(%f)=%f" %(_i + 1, iter_count, gene2f(gene, gene_size), gene_eval))

    # output the result
    gene, gene_eval = max(population, key = lambda x: x[1])
    print("result found: f(%f)=%f" %(gene2f(gene, gene_size), gene_eval))
    return

if __name__ == "__main__":
    main()
