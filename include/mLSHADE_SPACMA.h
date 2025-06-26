#pragma once
#ifndef MLSHADE_SPACMA_H
#define MLSHADE_SPACMA_H

#include <vector>
#include <algorithm>
#include <random>
#include <Eigen/Dense>
#include <functional>
#include <queue>

struct Individual {
    std::vector<double> position;
    double fitness;
    
    Individual(size_t dim, double init_value = 0.0)
        : position(dim, init_value), fitness(std::numeric_limits<double>::max()) {}

    bool operator<(const Individual& other) const {
        return fitness < other.fitness;
    }
};

struct Archive {
    std::vector<Individual> pop;
    size_t max_size;
    std::vector<double> funvalues;
    
    Archive(size_t size) : max_size(size) {}
};

class mLSHADE_SPACMA {
public:
    mLSHADE_SPACMA(size_t population_size, size_t dimension,
                  const std::vector<double>& min_bound, const std::vector<double>& max_bound,
                  size_t max_iter, double opt_value = 1e-30);
    
    void optimize(const std::function<double(const std::vector<double>&)>& loss_function);
    const Individual& getBestIndividual() const { return best_individual; }

private:
    size_t population_size;
    size_t dimension;
    std::vector<double> min_bound;
    std::vector<double> max_bound;
    size_t max_iter;
    double opt_value;

    std::vector<Individual> population;
    Individual best_individual;
    Archive archive;
    size_t current_nfes;  

    std::vector<double> memory_sf;
    std::vector<double> memory_cr;
    std::vector<double> memory_1st_class;
    size_t memory_pos;
  
    const double L_Rate = 0.8;     // study rate
    const double p_best_rate = 0.11;
    const double arc_rate = 1.4;   // archive rate
    const double fitness_tol = 1e-8;

    void initializePopulation();
    void exactEliminationGenerate(size_t elim_count);
    std::vector<double> mutate(size_t target_idx, double sf);
    std::vector<double> crossover(const std::vector<double>& target, const std::vector<double>& donor, double CR);
    bool updateArchive(const Individual& ind);
    void updateMemory(const std::vector<double>& good_sf, const std::vector<double>& good_cr,const std::vector<double>& dif_val);
    void reducePopulation();
    double generateRandom(double min, double max);
};

#endif // MLSHADE_SPACMA_H