// udp_cmd_vel_receiver.hpp
#ifndef UDP_CMD_VEL_RECEIVER_HPP
#define UDP_CMD_VEL_RECEIVER_HPP

#include <string>
#include <thread>
#include <mutex>
#include <functional>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace UDPCmdVel {

// 帧头和帧尾定义（4字节）
constexpr uint32_t FRAME_HEADER = 0xA5A5A5A5;
constexpr uint32_t FRAME_TAIL = 0x5A5A5A5A;

// 消息类型
enum MsgType : uint8_t {
    CMD_VEL = 0x01,
    HEARTBEAT = 0x02
};

struct CmdDataStruct{
    float linear_x;
    float linear_y;
    float linear_z;
    float angular_x;
    float angular_y;
    float angular_z;
};

class Receiver {
public:
    // 回调函数类型定义
    using CommandCallback = std::function<void(const CmdDataStruct&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    // 构造函数：指定服务器地址和端口
    Receiver(const std::string& server_ip = "192.168.19.101", 
             int server_port = 8888,
             int client_port = 9999)
        : server_ip_(server_ip), server_port_(server_port), client_port_(client_port), running_(false) {}

    // 析构函数
    ~Receiver() {
        stop();
    }

    // 启动接收器
    bool start(const CommandCallback& cmd_callback, 
               const ErrorCallback& error_callback = nullptr) {
        if (running_) return false;
        
        cmd_callback_ = cmd_callback;
        error_callback_ = error_callback;
        
        // 创建UDP套接字
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd_ < 0) {
            report_error("创建套接字失败");
            return false;
        }

        // 设置客户端地址
        memset(&client_addr_, 0, sizeof(client_addr_));
        client_addr_.sin_family = AF_INET;
        client_addr_.sin_addr.s_addr = INADDR_ANY;  // 所有可用接口
        client_addr_.sin_port = htons(client_port_);

        // 绑定套接字
        if (bind(sockfd_, (struct sockaddr*)&client_addr_, sizeof(client_addr_)) < 0) {
            report_error("绑定套接字失败");
            close(sockfd_);
            return false;
        }

        // 设置服务器地址
        memset(&server_addr_, 0, sizeof(server_addr_));
        server_addr_.sin_family = AF_INET;
        server_addr_.sin_addr.s_addr = inet_addr(server_ip_.c_str());
        server_addr_.sin_port = htons(server_port_);

        // 启动线程
        running_ = true;
        thread_ = std::thread(&Receiver::run, this);
        return true;
    }

    // 停止接收器
    void stop() {
        if (!running_) return;
        
        running_ = false;
        if (thread_.joinable()) {
            thread_.join();
        }
        
        if (sockfd_ >= 0) {
            close(sockfd_);
            sockfd_ = -1;
        }
    }

private:
    // 服务器信息
    std::string server_ip_;
    int server_port_;
    int client_port_;
    
    // 套接字和地址
    int sockfd_ = -1;
    sockaddr_in server_addr_;
    sockaddr_in client_addr_;
    
    // 线程和状态
    std::thread thread_;
    std::mutex mutex_;
    bool running_;
    
    // 回调函数
    CommandCallback cmd_callback_;
    ErrorCallback error_callback_;

    // 报告错误
    void report_error(const std::string& msg) {
        if (error_callback_) {
            std::lock_guard<std::mutex> lock(mutex_);
            error_callback_(msg);
        }
    }

    // 计算CRC32校验值
    uint32_t calculate_crc32(const uint8_t* data, size_t length) {
        // CRC32表（简化版）
        static const uint32_t crc_table[256] = {
                                        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
                                        0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
                                        0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
                                        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
                                        0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
                                        0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
                                        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
                                        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
                                        0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
                                        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
                                        0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
                                        0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
                                        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
                                        0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
                                        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
                                        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
                                        0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
                                        0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
                                        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
                                        0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
                                        0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
                                        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
                                        0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
                                        0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
                                        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
                                        0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
                                        0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
                                        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
                                        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
                                        0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
                                        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
                                        0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
                                    };
        
        uint32_t crc = 0xFFFFFFFF;
        
        for (size_t i = 0; i < length; ++i) {
            crc = (crc >> 8) ^ crc_table[(crc ^ data[i]) & 0xFF];
        }
        
        return crc ^ 0xFFFFFFFF;
    }

    // 线程主函数
    void run() {
        // 发送心跳包的线程
        std::thread heartbeat_thread([this]() {
            // 心跳包格式: [4B帧头] [4B数据长度(1)] [1B消息类型(HEARTBEAT)] [4B CRC32校验] [4B帧尾]
            const size_t heartbeat_size = 4 + 4 + 1 + 4 + 4;
            uint8_t heartbeat[heartbeat_size];
            
            // 设置帧头
            *((uint32_t*)heartbeat) = htonl(FRAME_HEADER);
            
            // 设置数据长度
            *((uint32_t*)(heartbeat + 4)) = htonl(1);
            
            // 设置消息类型
            heartbeat[8] = HEARTBEAT;
            
            // 计算并设置CRC32校验
            uint32_t crc = calculate_crc32(heartbeat + 8, 1);
            *((uint32_t*)(heartbeat + 9)) = htonl(crc);
            
            // 设置帧尾
            *((uint32_t*)(heartbeat + 13)) = htonl(FRAME_TAIL);
            
            while (running_) {
                sendto(sockfd_, heartbeat, heartbeat_size, 0, 
                      (struct sockaddr*)&server_addr_, sizeof(server_addr_));
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });

        // 接收数据的主循环
        uint8_t buffer[2048];
        sockaddr_in sender_addr;
        socklen_t sender_addr_len = sizeof(sender_addr);

        while (running_) {
            // 设置接收超时
            timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

            memset(buffer, 0, sizeof(buffer));
            int recv_len = recvfrom(sockfd_, buffer, sizeof(buffer), 0, 
                                   (struct sockaddr*)&sender_addr, &sender_addr_len);
            
            if (recv_len > 0) {
                // 解析数据包
                CmdDataStruct cmd;
                if (parsePacket(buffer, recv_len, cmd)) {
                    // 调用回调函数
                    if (cmd_callback_) {
                        std::lock_guard<std::mutex> lock(mutex_);
                        cmd_callback_(cmd);
                    }
                }
            }
        }

        // 等待心跳线程结束
        if (heartbeat_thread.joinable()) {
            heartbeat_thread.join();
        }
    }

    // 解析数据包（带帧头帧尾和校验）
    bool parsePacket(const uint8_t* data, size_t length, CmdDataStruct& cmd) {
        // 最小包长度: 帧头(4) + 长度(4) + 消息类型(1) + 校验(4) + 帧尾(4) = 17
        if (length < 17) {
            report_error("data length not enough");
            return false;
        }

        // 1. 检查帧头
        uint32_t header = ntohl(*((uint32_t*)data));
        if (header != FRAME_HEADER) {
            report_error("frame header wrong");
            return false;
        }

        // 2. 检查帧尾
        uint32_t tail = ntohl(*((uint32_t*)(data + length - 4)));
        if (tail != FRAME_TAIL) {
            report_error("frame tail wrong");
            return false;
        }

        // 3. 检查数据长度
        uint32_t data_length = ntohl(*((uint32_t*)(data + 4)));
        if (data_length + 16 != length) {  // 12 = 帧头(4) + 长度(4) + 校验(4) + 帧尾(4)
            report_error("data length not match");
            return false;
        }

        // 4. 检查消息类型
        uint8_t msg_type = data[8];
        if (msg_type != CMD_VEL) {
            // 非CMD_VEL消息，忽略
            return false;
        }

        // 5. 检查CRC32校验
        uint32_t received_crc = ntohl(*((uint32_t*)(data + 8 + data_length)));
        uint32_t calculated_crc = calculate_crc32(data + 8, data_length);
        if (received_crc != calculated_crc) {
            report_error("CRC32 failed");
            return false;
        }

        // 6. 解析速度数据（网络字节序转主机字节序）
        float cmd_data[6];
        memcpy(cmd_data, data + 9, sizeof(cmd_data));
        cmd.linear_x = cmd_data[0];
        cmd.linear_y = cmd_data[1];
        cmd.linear_z = cmd_data[2];
        cmd.angular_x = cmd_data[3];
        cmd.angular_y = cmd_data[4];
        cmd.angular_z = cmd_data[5];

        return true;
    }
};

} // namespace UDPCmdVel

#endif // UDP_CMD_VEL_RECEIVER_HPP