#include "id_manager.h"

// Reserve ID 0 to indicate a terminal connection (i.e. a port).
const int IdManager::kReservedTerminalId = 0;
int IdManager::next_id = 1;
int IdManager::prev_id = 0;



