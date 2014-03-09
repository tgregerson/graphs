#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <unordered_set>
#include <vector>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <pthread.h>

#include "id_manager.h"
#include "chaco_parser.h"
#include "directed_node.h"
#include "partition_engine.h"
#include "partition_engine_klfm.h"
#include "xml_parser.h"

#ifndef MAIN_NUM_THREADS 
  #define MAIN_NUM_THREADS 2
#endif

using std::vector;

struct KlfmThreadData {
  int thread_id;
  PartitionEngineKlfm* engine;
  vector<PartitionSummary> summaries;
};

void* RunKlfmThread(void* klfm_td) {
  KlfmThreadData* klfm_data = (KlfmThreadData*)klfm_td;
  printf("Starting execution on thread %d\n", klfm_data->thread_id);
  klfm_data->engine->Execute(&(klfm_data->summaries));
  delete klfm_data->engine;
  klfm_data->engine = NULL;
  printf("Finished execution on thread %d\n", klfm_data->thread_id);
}

int main(int argc, char *argv[]) {
  xmlKeepBlanksDefault(0);
  char *filename = NULL;

  if (argc != 2) {
    printf("\nIncorrect number of arguments\n");
    printf("Use: partition <graph.xml>\n\n");
    exit(1);
  }
  std::string temp;

  filename = argv[1];

  Node* graph;
  graph = new DirectedNode(9999999, 1.0, "Top-Level Graph");
  //XmlParser parser;
  {
    ChacoParser parser;
    assert(parser.Parse(graph, filename));
  }

  /*
  std::unordered_set<int> debug;
  debug.insert(4);
  debug.insert(5);
  debug.insert(7);
  graph.ConsolidateSubNodes(debug);
  graph.Print(true);

  debug.clear();
  debug.insert(6);
  debug.insert(113);
  graph.ConsolidateSubNodes(debug);
  graph.Print(true);

  assert(graph.ExpandSupernode(120));
  assert(graph.ExpandSupernode(113));
  graph.Print(true); */

  pthread_t threads[MAIN_NUM_THREADS];
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  KlfmThreadData thread_data[MAIN_NUM_THREADS];
  PartitionEngineKlfm::Options options;
  //options.seed_mode =
  //    PartitionEngineKlfm::Options::kSeedModeSimpleDeterministic;

  for (int thread_id = 0; thread_id < MAIN_NUM_THREADS; ++thread_id) {
    printf("Creating partition engine %d of %d\n", thread_id + 1,
            MAIN_NUM_THREADS);
    thread_data[thread_id].engine = new PartitionEngineKlfm(graph, options, NULL);
  }
  delete graph;

  timespec ts, rem;
  ts.tv_sec = 1;
  ts.tv_nsec = 0;
  int ret;
  for (int thread_id = 0; thread_id < MAIN_NUM_THREADS; ++thread_id) {
    printf("Creating thread %d of %d\n", thread_id + 1,
            MAIN_NUM_THREADS);
    thread_data[thread_id].thread_id = thread_id;
    ret = pthread_create(&threads[thread_id], &attr, RunKlfmThread,
          (void*)(&thread_data[thread_id]));
    assert(ret == 0);
    nanosleep(&ts, &rem);
  }

  pthread_attr_destroy(&attr);

  void* status;
  for (int thread_id = 0; thread_id < MAIN_NUM_THREADS; ++thread_id) {
    printf("Waiting for thread %d to finish...\n", thread_id);
    ret = pthread_join(threads[thread_id], &status);
    assert(ret == 0);
  }
  //summary.Print();

  printf("All threads finished\n\n");

  printf("Results from each run:\n");
  for (int i = 0; i < MAIN_NUM_THREADS; ++i) {
    for (auto it : thread_data[i].summaries) {
      printf("%d\n", it.total_cost);
    }
  }
  pthread_exit(NULL);
}
