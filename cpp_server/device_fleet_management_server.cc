
// device_fleet_management_server.cpp

#include "device_fleet_management.h"           // Must declare DevicePool, Device, etc.
#include <grpcpp/grpcpp.h>
#include "device_fleet_management.grpc.pb.h"

#include <string>
#include <future>
#include <memory>
#include <iostream>

// Prefer fully qualified std::string
int InitiateDeviceAction(int device_id, int action_type, const std::string& action_param) {
    // Delegate to DevicePool — must be thread-safe
    return DevicePool::getInstance()->Initiate(device_id, action_type, action_param);
}

class DeviceFleetManagementImpl final
    : public devicefleetmanagement::DeviceFleetManagement::Service {

public:
    grpc::Status RegisterDevice(grpc::ServerContext* context,
                                const devicefleetmanagement::Device* request,
                                devicefleetmanagement::NoParam* reply) override
    {
        try {
            DevicePool::getInstance()->RegisterDevice(request->device_id(), request->state());
            return grpc::Status::OK;
        } catch (const std::exception& ex) {
            return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        }
    }

    grpc::Status SetDeviceStatus(grpc::ServerContext* context,
                                 const devicefleetmanagement::Device* request,
                                 devicefleetmanagement::NoParam* reply) override
    {
        try {
            bool ok = DevicePool::getInstance()->SetDeviceStatus(request->device_id(), request->state());
            if (!ok) {
                return grpc::Status(grpc::StatusCode::NOT_FOUND, "Device not found");
            }
            return grpc::Status::OK;
        } catch (const std::exception& ex) {
            return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        }
    }

    grpc::Status GetDeviceInfo(grpc::ServerContext* context,
                               const devicefleetmanagement::GetDeviceInfoRequest* request,
                               devicefleetmanagement::GetDeviceInfoReply* reply) override
    {
        try {
            Device* dev = DevicePool::getInstance()->GetDevice(request->device_id());
            if (dev == nullptr) {
                return grpc::Status(grpc::StatusCode::NOT_FOUND, "Device not found");
            }
            // Use consistent getters
            reply->set_current_state(dev->get_state()); // NOTE: adjust to your proto (current_state vs currentl_state)
            reply->set_info(dev->get_info());
            return grpc::Status::OK;
        } catch (const std::exception& ex) {
            return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        }
    }

    grpc::Status InitiateDeviceAction(grpc::ServerContext* context,
                                      const devicefleetmanagement::InitiateDeviceActionRequest* request,
                                      devicefleetmanagement::InitiateDeviceActionReply* reply) override
    {
        try {
            Device* dev = DevicePool::getInstance()->GetDevice(request->device_id());
            if (dev == nullptr) {
                return grpc::Status(grpc::StatusCode::NOT_FOUND, "Device not found");
            }

            // Return current state
            reply->set_state(dev->get_state());

            // Kick off the action in background and return action_id immediately
            // Avoid blocking with future.get() unless required by API
            int action_id = DevicePool::getInstance()->Initiate(
                request->device_id(), request->action_type(), request->action_param()
            );

            reply->set_action_id(action_id);
            return grpc::Status::OK;

        } catch (const std::exception& ex) {
            return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        }
    }

    grpc::Status GetDeviceAction(grpc::ServerContext* context,
                                 const devicefleetmanagement::GetDeviceActionRequest* request,
                                 devicefleetmanagement::GetDeviceActionReply* reply) override
    {
        try {
            auto status = DevicePool::getInstance()->GetDeviceAction(
                request->device_id(), request->action_id()
            );
            reply->set_action_status(status);
            return grpc::Status::OK;
        } catch (const std::exception& ex) {
            return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
        }
    }
};

void RunServer() {
    const std::string server_address("0.0.0.0:50051");
    DeviceFleetManagementImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    RunServer();
    return 0;
}

