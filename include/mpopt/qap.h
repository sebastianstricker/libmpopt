#ifndef LIBMPOPT_QAP_H
#define LIBMPOPT_QAP_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mpopt_qap_solver_t mpopt_qap_solver;
typedef struct mpopt_qap_graph_t mpopt_qap_graph;
typedef struct mpopt_qap_unary_node_t mpopt_qap_unary_node;
typedef struct mpopt_qap_uniqueness_node_t mpopt_qap_uniqueness_node;
typedef struct mpopt_qap_pairwise_node_t mpopt_qap_pairwise_node;

// 
/* memory_kb: Approximate memory size needed to store the libmpopt graph representation of the given qap.
* Libmpopt uses a custom block allocator that ensures its variables are stored sequentially in memory.
* To ensure pointers remain valid, the block is not allowed to be moved after addding nodes to the libmpopt graph.
* Therefore, the memory needed has to be given during initialization of the memory block.
*
* Due to the potential sparsity of the QAP problem, we cannot estimate this acurately through the number of unary and pairwise costs of the original QAP.
* However, the amount of memory can be estimated through the number of calls to this interface's allocating functions. 
*
* Memory is allocated when inserting unary, uniqueness and pairwise nodes. You can inspect and follow these three interface functions:
* 1) mpopt_qap_unary_node* mpopt_qap_graph_add_unary(mpopt_qap_graph* graph, int idx, int number_of_labels, int number_of_forward, int number_of_backward);
* 2) mpopt_qap_uniqueness_node* mpopt_qap_graph_add_uniqueness(mpopt_qap_graph* graph, int idx, int number_of_unaries, int label_idx);
* 3) pairwise_node_type* add_pairwise(index idx, index number_of_labels0, index number_of_labels1);
*
* Each function reserves a sequence of fixed_vector (fixed_vector.hpp) objects, containing the cost values (currently doubles).
* 
* Following the functions they require the following amount of memory space: 
*       1) sizeof(double) * (no_connections * 2 + number_of_forward + number_of_backward)
*       2) sizeof(double) * (number_of_unaries * 2) + 1
*       3) sizeof(double) * (number_of_labels0 * number_of_labels1)
* 
* Where the + 1 of 2) stems from the additional dummy node needed to allow for nodes to be left unmatched.
*
* I recommend to count this memory requirement and multiply by a factor of 2,
* as one needs to account for the overhead of the fixed_vector containing these cost values
* and possible lost memory due to the alignment of inserts into the block memory. (see align() function in allocator.hpp).
*
* Ideally this estimation should happen within the library. However, I decided to keep the code as much 'as is' as possible 
* and write an adapter on the including parent project's side to handle this requirement.
*
* Sebastian Stricker.
*/
mpopt_qap_solver* mpopt_qap_solver_create(size_t memory_kb);
void mpopt_qap_solver_destroy(mpopt_qap_solver* s);
mpopt_qap_graph* mpopt_qap_solver_get_graph(mpopt_qap_solver* s);
mpopt_qap_unary_node* mpopt_qap_graph_add_unary(mpopt_qap_graph* graph, int idx, int number_of_labels, int number_of_forward, int number_of_backward);
mpopt_qap_uniqueness_node* mpopt_qap_graph_add_uniqueness(mpopt_qap_graph* graph, int idx, int number_of_unaries, int label_idx);
mpopt_qap_pairwise_node* mpopt_qap_graph_add_pairwise(mpopt_qap_graph* graph, int idx, int number_of_labels0, int number_of_labels1);
void mpopt_qap_graph_add_pairwise_link(mpopt_qap_graph* graph, int idx_unary0, int idx_unary1, int idx_pairwise);
void mpopt_qap_graph_add_uniqueness_link(mpopt_qap_graph* graph, int idx_unary, int label, int idx_uniqueness, int slot);
mpopt_qap_unary_node* mpopt_qap_graph_get_unary(mpopt_qap_graph* graph, int idx);
mpopt_qap_uniqueness_node* mpopt_qap_graph_get_uniqueness(mpopt_qap_graph* graph, int idx);
mpopt_qap_pairwise_node* mpopt_qap_graph_get_pairwise(mpopt_qap_graph* graph, int idx);
void mpopt_qap_solver_set_fusion_moves_enabled(mpopt_qap_solver* s, bool enabled);
void mpopt_qap_solver_set_dual_updates_enabled(mpopt_qap_solver* s, bool enabled);
//void mpopt_qap_solver_set_stopping_criterion(mpopt_qap_solver* s, float epsilon_lb, float epsilon_ub, int k_batches);
void mpopt_qap_solver_set_stopping_criterion(mpopt_qap_solver* s, float p, int k_batches);
void mpopt_qap_solver_set_local_search_enabled(mpopt_qap_solver* s, bool enabled);
void mpopt_qap_solver_set_grasp_alpha(mpopt_qap_solver* s, double alpha);
void mpopt_qap_solver_use_grasp(mpopt_qap_solver* s);
void mpopt_qap_solver_use_greedy(mpopt_qap_solver* s);
void mpopt_qap_solver_set_random_seed(mpopt_qap_solver* s, const unsigned long seed);
void mpopt_qap_solver_run(mpopt_qap_solver* s, int batch_size, int max_batches, int greedy_generations);
void mpopt_qap_solver_solve_ilp(mpopt_qap_solver* s);
void mpopt_qap_solver_execute_combilp(mpopt_qap_solver* s);
void mpopt_qap_solver_compute_greedy_assignment(mpopt_qap_solver* s);
double mpopt_qap_solver_runtime(mpopt_qap_solver* t);
double mpopt_qap_solver_lower_bound(mpopt_qap_solver* s);
double mpopt_qap_solver_evaluate_primal(mpopt_qap_solver* s);

void mpopt_qap_unary_set_cost(mpopt_qap_unary_node* n, int label, double cost);
double mpopt_qap_unary_get_cost(mpopt_qap_unary_node* n, int label);
int mpopt_qap_unary_get_primal(mpopt_qap_unary_node* n);

void mpopt_qap_uniqueness_set_cost(mpopt_qap_uniqueness_node* n, int unary, double cost);
double mpopt_qap_uniqueness_get_cost(mpopt_qap_uniqueness_node* n, int unary);
int mpopt_qap_uniqueness_get_primal(mpopt_qap_uniqueness_node* n);

void mpopt_qap_pairwise_set_cost(mpopt_qap_pairwise_node* n, int l0, int l1, double cost);
double mpopt_qap_pairwise_get_cost(mpopt_qap_pairwise_node* n, int l0, int l1);
int mpopt_qap_pairwise_get_primal(mpopt_qap_pairwise_node* n, char left_side);


#ifdef __cplusplus
}
#endif

#endif

/* vim: set ts=8 sts=2 sw=2 et ft=c: */
