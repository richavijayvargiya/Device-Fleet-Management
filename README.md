# Device Fleet Management

A minimal, extensible system for managing a fleet of devices using:
- **C++ Backend Service** (gRPC-style)
- **Python CLI Client**

Includes a clear **Protocol Buffers contract** and reference implementations for local development and experimentation.

---

## ✅ Features
- In-memory device registry (reference implementation)
- CRUD-style API for devices & actions via gRPC-style service
- Minimal **C++ server**
- Lightweight **Python CLI client**
- Clear `.proto` contract for generating language-specific stubs

---

## 🔧 Prerequisites
For Linux (Ubuntu/Debian):
- **C++17** compiler (GCC 7+, Clang 5+)
- **CMake 3.15+**
- **Protocol Buffers (protoc) 3.0+**
- **gRPC C++ library**
- **Python 3.7+** & `pip`

Install essentials:
```bash
sudo apt-get update
sudo apt-get install -y build-essential python3 python3-venv python3-pip protobuf-compiler
```

---

## 📂 Project Structure
```
DeviceFleetManagement/
├── cpp_server/
│   ├── device_fleet_management_server.cc
│   └── device_fleet_management.h
├── python_cli/
│   └── device_fleet_management_client.py
├── proto/
│   └── device_fleet_management.proto
└── README.md
```

---

## ▶️ Build & Run

### First, compile the Protocol Buffers:

#### For C++ Server
```bash
protoc -I proto/ \
  --cpp_out=server/src/ \
  --grpc_out=server/src/ \
  --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
  proto/device_service.proto

protoc -I proto device_fleet_management.proto \
  --grpc_out=cpp_server --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` \
  --cpp_out=cpp_server

### **C++ Backend**
```bash
cd cpp_server
g++ -std=c++17 device_fleet_management_server.cc -o device_fleet_management_server -pthread
./device_fleet_management_server
```
Default port: `0.0.0.0:50051` (adjust in source if needed).

---

### **Python CLI**
```bash
cd python_cli
chmod +x device_fleet_management_client.py
./device_fleet_management_client.py
```

---

## 📜 Protocol Buffers
Defined in `proto/device_fleet_management.proto`.  
Generate stubs using `protoc` + language-specific plugins.

---

## 🏗 Architecture

**Flow:**
1. **Python CLI** sends request → **C++ Backend**
2. Backend processes request → updates **In-Memory Registry**
3. Response returned to CLI

---

## 💻 CLI Examples
Menu:
```
1. RegisterDevice
2. RemoveDevice
3. ListDevices
4. GetDeviceAction
5. UpdateDeviceAction
```

Example:
```
[RegisterDevice] success=true message="Device registered"
```

---

## 🚀 Next Steps
- **Error Handling**: Current version has minimal error handling. Add robust exception handling, input validation, and negative-path tests.
- **GetDeviceAction**: Currently uses only `action_id`. Adding `device_id` is recommended for clarity and maintainability.
- **CLI UX**: Implement a cleaner infinite loop, better prompts, and validation for numeric menu selection.
- **State Management**: In-memory structures assumed. Consider persistence (SQLite/PostgreSQL) for production environments.
- **Authentication**: None in current version. Add API keys or OAuth for secure operations if exposed remotely.
- **Logging & Observability**: Add structured logging, metrics, and tracing for debugging and monitoring.
- **Containerization**: Provide Dockerfiles and CI/CD workflows for reproducible builds and deployments.

---

## 🛠 Troubleshooting
- Ensure server is running before CLI
- Check host/port configuration
- Verify firewall settings

---


