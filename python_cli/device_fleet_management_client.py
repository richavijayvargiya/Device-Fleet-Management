
#!/usr/bin/env python3
import grpc
from google.protobuf import empty_pb2
import device_fleet_management_pb2 as pb
import device_fleet_management_pb2_grpc as stubs


def device_status_name(value: int) -> str:
    # Converts enum int to name: e.g., 4 -> "STATUS_UPDATING"
    try:
        return pb.DeviceStatus.Name(value)
    except ValueError:
        return f"UNKNOWN({value})"


def action_status_name(value: int) -> str:
    try:
        return pb.ActionStatus.Name(value)
    except ValueError:
        return f"UNKNOWN({value})"


def print_menu():
    print(
        """\nPlease select (1-5):
  1. RegisterDevice
  2. SetDeviceStatus
  3. GetDeviceInfo
  4. InitiateDeviceAction
  5. GetDeviceAction
  Any other number to exit
"""
    )


def run_client():
    with grpc.insecure_channel("localhost:50051") as channel:
        stub = stubs.DeviceFleetManagementStub(channel)

        while True:
            try:
                print_menu()
                choice = int(input("Enter choice (1-5): ").strip())

                if choice == 1:
                    # RegisterDevice
                    d_id = int(input("Enter Device ID: ").strip())
                    print(
                        """Enter status (0-6):
  STATUS_IDLE = 0
  STATUS_BUSY = 1
  STATUS_OFFLINE = 2
  STATUS_MAINTENANCE = 3
  STATUS_UPDATING = 4
  STATUS_RECOVERING = 5
  STATUS_ERROR = 6
"""
                    )
                    st = int(input("Enter status: ").strip())
                    req = pb.Device(device_id=d_id, status=st)
                    _ = stub.RegisterDevice(req)  # returns Empty
                    print(f"✔ Registered device {d_id} with status {device_status_name(st)}")

                elif choice == 2:
                    # SetDeviceStatus
                    d_id = int(input("Enter Device ID: ").strip())
                    print(
                        """Enter status (0-6):
  STATUS_IDLE = 0
  STATUS_BUSY = 1
  STATUS_OFFLINE = 2
  STATUS_MAINTENANCE = 3
  STATUS_UPDATING = 4
  STATUS_RECOVERING = 5
  STATUS_ERROR = 6
"""
                    )
                    st = int(input("Enter status: ").strip())
                    req = pb.Device(device_id=d_id, status=st)
                    _ = stub.SetDeviceStatus(req)  # returns Empty
                    print(f"✔ Updated device {d_id} to {device_status_name(st)}")

                elif choice == 3:
                    # GetDeviceInfo
                    d_id = int(input("Enter Device ID: ").strip())
                    req = pb.GetDeviceInfoRequest(device_id=d_id)
                    res = stub.GetDeviceInfo(req)
                    print(
                        f"ℹ Device {d_id} | State: {device_status_name(res.current_state)} | Info: {res.info}"
                    )

                elif choice == 4:
                    # InitiateDeviceAction
                    d_id = int(input("Enter Device ID: ").strip())
                    print("Select ActionType:\n  SOFTWARE_UPDATE = 0")
                    a_type = int(input("Enter ActionType: ").strip())
                    a_param = input("Enter Action Param (e.g., software version): ").strip()
                    req = pb.InitiateDeviceActionRequest(
                        device_id=d_id, action_type=a_type, action_param=a_param
                    )
                    res = stub.InitiateDeviceAction(req)
                    print(
                        f"▶ Initiated action {res.action_id} on device {d_id}; device state: {device_status_name(res.state)}"
                    )

                elif choice == 5:
                    # GetDeviceAction
                    d_id = int(input("Enter Device ID: ").strip())
                    a_id = int(input("Enter Action ID: ").strip())
                    req = pb.GetDeviceActionRequest(device_id=d_id, action_id=a_id)
                    res = stub.GetDeviceAction(req)
                    print(f"⏱ Action {a_id} status: {action_status_name(res.action_status)}")

                else:
                    print("Bye!")
                    break

            except grpc.RpcError as e:
                # Show gRPC status code + details from server
                code = e.code().name if hasattr(e.code(), "name") else str(e.code())
                print(f"gRPC error: {code} - {e.details()}")

            except ValueError:
                print("Invalid input. Please enter numeric values where required.")

            except Exception as e:
                print(f"Error: {e}")


if __name__ == "__main__":
    run_client()

