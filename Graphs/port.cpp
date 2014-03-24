#include "port.h"

#include "id_manager.h"

Port::Port(int id)
  : id(id), internal_edge_id(IdManager::kReservedTerminalId),
    external_edge_id(IdManager::kReservedTerminalId), type(kDontCareType) {}

void Port::Print() const {
  printf("Port  ID=%d  Name=%s  Type=", id, name.c_str()); 
  if (type == Port::kInputType) {
    printf("Input\n");
  } else {
    printf("Output\n");
  }
  printf("External Edge ID: ");
  if (external_edge_id == IdManager::kReservedTerminalId) {
    printf("TERMINAL");
  } else {
    printf("%d", external_edge_id);
  }
  printf("\n");
  printf("Internal Edge ID: %d\n", internal_edge_id);
}
