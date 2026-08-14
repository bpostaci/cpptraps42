#pragma once
struct Configuration { int value; Configuration(); };
extern Configuration global_configuration;
extern int copied_during_static_initialization;
const Configuration& safe_configuration();
