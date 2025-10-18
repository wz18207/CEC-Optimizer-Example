#include "mLSHADE_SPACMA.h"
#include <cmath>
#include <numeric>
#include <queue>

mLSHADE_SPACMA::mLSHADE_SPACMA(size_t population_size, size_t dimension,const std::vector<double>& min_bound, const std::vector<double>& max_bound,size_t max_iter, double opt_value)
    : population_size(population_size), 
      dimension(dimension),
      min_bound(min_bound), 
      max_bound(max_bound), 
      max_iter(max_iter),
      opt_value(opt_value),
      best_individual(dimension),
      archive(static_cast<size_t>(arc_rate * population_size)) {
    memory_sf = std::vector<double>(dimension, 0.5);
    memory_cr = std::vector<double>(dimension, 0.5);
    memory_1st_class = std::vector<double>(dimension, 0.5);
    memory_pos = 0;
    current_nfes = 0;
}

void mLSHADE_SPACMA::initializePopulation() {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (size_t i = 0; i < population_size; ++i) {
        Individual ind(dimension);
        for (size_t j = 0; j < dimension; ++j) {
            std::uniform_real_distribution<> dis(min_bound[j], max_bound[j]);
            ind.position[j] = dis(gen);
        }
        population.push_back(ind);
    }
}

void mLSHADE_SPACMA::exactEliminationGenerate(size_t elim_count) {
    std::sort(population.begin(), population.end());
    for (size_t i = 0; i < elim_count; ++i) {
        population.pop_back();
    }
    
    // x_best1 + rand*(x_best1 - x_best2)
    std::uniform_real_distribution<> dis(0.0, 1.0);
    std::mt19937 gen(std::random_device{}());
    
    for (size_t i = 0; i < elim_count; ++i) {
        Individual new_ind(dimension);
        
        for (size_t j = 0; j < dimension; ++j) {
            double diff = population[0].position[j] - population[1].position[j];
            new_ind.position[j] = population[0].position[j] + dis(gen) * diff;
            new_ind.position[j] = std::max(min_bound[j], std::min(max_bound[j], new_ind.position[j]));
        }
        population.push_back(new_ind);
    }
}

std::vector<double> mLSHADE_SPACMA::mutate(size_t target_idx, double sf) {
    std::vector<double> mutant(dimension);
    size_t pbest_idx = static_cast<size_t>(population_size * p_best_rate * generateRandom(0, 1));
    pbest_idx = std::min(pbest_idx, population_size - 1);
    std::vector<size_t> indices(population_size);
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        return population[a].fitness < population[b].fitness;
    });
    
    std::discrete_distribution<size_t> rank_dist(indices.size(), [&](size_t i) {
        return indices.size() - i; 
    });
    
    size_t pr1_idx = indices[rank_dist(std::mt19937(std::random_device{}()))];
    size_t r2_idx;
    do {
        r2_idx = generateRandom(0, population_size - 1);
    } while (r2_idx == target_idx);
    
    // v_i = x_i + F_i*(x_pbest - x_i) + F_i*(x_pr1 - x_r2)
    for (size_t i = 0; i < dimension; ++i) {
        mutant[i] = population[target_idx].position[i] +sf * (population[pbest_idx].position[i] - population[target_idx].position[i]) + sf * (population 
        [pr1_idx].position[i] - population[r2_idx].position[i]);
        mutant[i] = std::max(min_bound[i], std::min(max_bound[i], mutant[i]));
    }
    
    return mutant;
}

std::vector<double> mLSHADE_SPACMA::crossover(const std::vector<double>& target, const std::vector<double>& donor, double CR) {
    std::vector<double> offspring(dimension);
    size_t j_rand = generateRandom(0, dimension - 1);
    for (size_t i = 0; i < dimension; ++i) {
        if (generateRandom(0, 1) < CR || i == j_rand) {
            offspring[i] = donor[i];
        } else {
            offspring[i] = target[i];
        }
    }
    return offspring;
}

bool mLSHADE_SPACMA::updateArchive(const Individual& ind) {
    if (archive.pop.size() >= archive.max_size) {
        auto worst_it = std::max_element(archive.pop.begin(), archive.pop.end(),[](const Individual& a, const Individual& b) {
            return a.fitness < b.fitness;
        });
        if (ind.fitness < worst_it->fitness) {
            *worst_it = ind;
            return true;
        }
        return false;
    }
    archive.pop.push_back(ind);
    return true;
}

void mLSHADE_SPACMA::updateMemory(const std::vector<double>& good_sf, const std::vector<double>& good_cr, const std::vector<double>& dif_val) {
    if (good_sf.empty()) return;
    double sum_dif = std::accumulate(dif_val.begin(), dif_val.end(), 0.0);
    double new_sf = 0.0;
    double new_cr = 0.0;
    
    for (size_t i = 0; i < good_sf.size(); ++i) {
        new_sf += dif_val[i] * good_sf[i] * good_sf[i];
        new_cr += dif_val[i] * good_cr[i] * good_cr[i];
    }
    
    new_sf /= (sum_dif * std::accumulate(good_sf.begin(), good_sf.end(), 0.0));
    new_cr /= (sum_dif * std::accumulate(good_cr.begin(), good_cr.end(), 0.0));
    memory_sf[memory_pos] = new_sf;
    memory_cr[memory_pos] = (new_cr == 0 || memory_cr[memory_pos] == -1) ? -1 : new_cr;
    memory_pos = (memory_pos + 1) % dimension;
}
void mLSHADE_SPACMA::reducePopulation() {
    if (population_size <= 5) return;  
    
    size_t new_size = population_size - 1;  
    std::sort(population.begin(), population.end());  

    population.erase(population.begin() + new_size, population.end());
    population_size = new_size;
}

void mLSHADE_SPACMA::optimize(const std::function<double(const std::vector<double>&)>& loss_function) {
    initializePopulation();
    for (auto& ind : population) {
        ind.fitness = loss_function(ind.position);
        current_nfes++;
        if (ind.fitness < best_individual.fitness) {
            best_individual = ind;
        }
    }

    for (size_t iter = 0; iter < max_iter && current_nfes < max_iter * population_size; ++iter) {
        if (current_nfes < (max_iter * population_size) / 2) {
            size_t elim_count = std::ceil(0.01 * population_size);  
            exactEliminationGenerate(elim_count);
        }
        
        std::vector<double> sf_values;
        std::vector<double> cr_values;
        std::vector<Individual> new_population;
        for (size_t i = 0; i < population_size; ++i) {
            size_t mem_idx = static_cast<size_t>(generateRandom(0, dimension - 1));
            double sf;
            if (current_nfes < (max_iter * population_size) / 2) {
                sf = 0.5 + 0.1 * generateRandom(0, 1);  
            } else {
                sf = memory_sf[mem_idx] + 0.1 * std::tan(M_PI * (generateRandom(0, 1) - 0.5));
                sf = std::max(0.1, std::min(1.0, sf));  
            }
            double cr = std::normal_distribution<>(memory_cr[mem_idx], 0.1)(std::mt19937(std::random_device{}()));
            cr = std::max(0.0, std::min(1.0, cr));
            auto donor = mutate(i, sf);
            auto offspring = crossover(population[i].position, donor, cr);
            Individual new_ind(dimension);
            new_ind.position = offspring;
            new_ind.fitness = loss_function(offspring);
            current_nfes++;
            if (new_ind.fitness < population[i].fitness) {
                new_population.push_back(new_ind);
                if (new_ind.fitness < best_individual.fitness) {
                    best_individual = new_ind;
                }
                updateArchive(population[i]);
                sf_values.push_back(sf);
                cr_values.push_back(cr);
            } 
            else {
                new_population.push_back(population[i]);
            }
        }
        
        population = new_population;
        if (!sf_values.empty()) {
            std::vector<double> dif_values(sf_values.size(), 1.0); 
            updateMemory(sf_values, cr_values, dif_values);
        }
        if (iter % 10 == 0) {
            reducePopulation();
        }
        if (std::abs(best_individual.fitness - opt_value) < fitness_tol) {
            break;
        }
    }
}

double mLSHADE_SPACMA::generateRandom(double min, double max) {
    std::uniform_real_distribution<> dis(min, max);
    return dis(std::mt19937(std::random_device{}()));
}