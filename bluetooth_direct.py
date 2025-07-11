# bluetooth_direct.py
import serial
import time
import threading
from openai import OpenAI

class BluetoothDirectChat:
    def __init__(self, api_key):
        """
        初始化蓝牙直接聊天器
        :param api_key: DeepSeek API密钥
        """
        self.serial_port = None
        self.api_key = api_key
        self.running = False
        self.bt_port = None
        
        # 初始化OpenAI客户端
        self.client = OpenAI(api_key=api_key, base_url="https://api.deepseek.com/v1")
        
        # 找到蓝牙串口
        self.find_bluetooth_port()
    
    def find_bluetooth_port(self):
        """连接HC05蓝牙模块 - 直接使用COM11"""
        port = "COM11"
        
        print(f"🔍 正在连接HC05蓝牙模块: {port}")
        print("📋 波特率: 9600 (HC05标准波特率)")
        
        try:
            print(f"🔌 连接串口: {port}")
            self.serial_port = serial.Serial(
                port=port,
                baudrate=9600,  # HC05蓝牙模块标准波特率
                bytesize=8,
                parity='N',
                stopbits=1,
                timeout=2
            )
            
            print("✅ 串口连接成功！")
            
            # 发送AT命令测试蓝牙模块
            print("📤 发送AT命令测试...")
            self.serial_port.write(b"AT\r\n")
            time.sleep(1)
            
            # 读取回应
            if self.serial_port.in_waiting > 0:
                response = self.serial_port.read(self.serial_port.in_waiting).decode('utf-8', errors='ignore')
                print(f"📡 HC05回应: '{response.strip()}'")
                
                if "OK" in response:
                    print("✅ HC05蓝牙模块响应正常")
                else:
                    print("⚠️  HC05回应异常，但连接已建立")
            else:
                print("⚠️  未收到AT命令回应，但连接已建立")
            
            self.bt_port = port
            return True
            
        except Exception as e:
            print(f"❌ 连接COM11失败: {e}")
            print("💡 请检查：")
            print("1. HC05蓝牙模块是否连接到COM11")
            print("2. COM11是否被其他程序占用")
            print("3. STM32是否已上电运行")
            return False
    
    def send_at_command(self, command):
        """发送AT命令"""
        try:
            print(f"📤 发送AT命令: {command}")
            self.serial_port.write(f"{command}\r\n".encode('utf-8'))
            time.sleep(1)
            
            if self.serial_port.in_waiting > 0:
                response = self.serial_port.read(self.serial_port.in_waiting).decode('utf-8', errors='ignore')
                print(f"📡 HC05回应: '{response.strip()}'")
                return response
            else:
                print("❌ 无回应")
                return None
        except Exception as e:
            print(f"❌ 发送AT命令失败: {e}")
            return None
    
    def call_deepseek_api(self, question):
        """调用DeepSeek API"""
        try:
            print("🤖 正在调用DeepSeek API...")
            response = self.client.chat.completions.create(
                model="deepseek-chat",
                messages=[
                    {"role": "system", "content": "You are an intelligent assistant. Please use concise and friendly English. Be sure to answer questions in English, and keep your answers to 20 words or less."},
                    {"role": "user", "content": question}
                ],
                stream=False,
                max_tokens=200,
                temperature=0.7
            )
            
            answer = response.choices[0].message.content
            return answer.strip()
            
        except Exception as e:
            return f"API调用失败: {str(e)[:50]}..."
    
    def send_to_phone(self, message):
        """直接发送消息到手机"""
        try:
            # 添加标识前缀，方便在手机上识别
            full_message = f"AI_Direct: {message}\r\n"
            
            print(f"📱 发送到手机: '{message}'")
            print(f"📝 完整格式: {repr(full_message)}")
            
            self.serial_port.write(full_message.encode('utf-8'))
            self.serial_port.flush()
            
            print("✅ 消息已发送到蓝牙")
            print("💡 请检查手机是否收到消息")
            
            return True
        except Exception as e:
            print(f"❌ 发送失败: {e}")
            return False
    
    def test_bluetooth_connection(self):
        """测试蓝牙连接"""
        print("\n" + "="*50)
        print("🧪 蓝牙连接测试")
        print("="*50)
        
        # 测试AT命令
        commands = [
            "AT",           # 基本测试
            "AT+VERSION",   # 获取版本
            "AT+ROLE?",     # 查询角色
            "AT+STATE?",    # 查询状态
        ]
        
        for cmd in commands:
            self.send_at_command(cmd)
            time.sleep(1)
        
        # 发送测试消息
        print("\n📱 发送测试消息到手机...")
        test_messages = [
            "Hello from Python!",
            "Bluetooth test message",
            "AI chat system ready"
        ]
        
        for msg in test_messages:
            self.send_to_phone(msg)
            time.sleep(2)
    
    def run_interactive_mode(self):
        """运行交互模式"""
        print("\n" + "="*60)
        print("🚀 蓝牙直接聊天模式已启动")
        print("="*60)
        print("📋 使用方法：")
        print("1. 输入 'at:命令' 发送AT命令")
        print("2. 输入 'send:消息' 直接发送消息到手机")
        print("3. 输入 'ask:问题' 调用AI并发送答案到手机")
        print("4. 输入 'test' 运行蓝牙连接测试")
        print("5. 输入 'quit' 退出程序")
        print("="*60)
        
        # 启动蓝牙接收监听线程
        def bluetooth_receive_thread():
            """监听蓝牙接收数据"""
            while self.running:
                try:
                    if self.serial_port.in_waiting > 0:
                        data = self.serial_port.read(self.serial_port.in_waiting).decode('utf-8', errors='ignore')
                        if data.strip():
                            print(f"\n📡 蓝牙接收: '{data.strip()}'")
                            
                            # 检查是否是问题
                            if '?' in data:
                                question = data.strip()
                                print(f"❓ 检测到问题: {question}")
                                
                                # 自动调用AI处理
                                answer = self.call_deepseek_api(question)
                                print(f"💡 AI回答: {answer}")
                                
                                # 发送答案到手机
                                self.send_to_phone(f"Answer: {answer}")
                            
                            print(">>> ", end="", flush=True)
                    time.sleep(0.1)
                except:
                    break
        
        self.running = True
        receive_thread = threading.Thread(target=bluetooth_receive_thread)
        receive_thread.daemon = True
        receive_thread.start()
        
        try:
            while True:
                user_input = input(">>> ").strip()
                
                if user_input.lower() == 'quit':
                    break
                elif user_input.lower() == 'test':
                    self.test_bluetooth_connection()
                elif user_input.lower().startswith('at:'):
                    cmd = user_input[3:].strip()
                    self.send_at_command(cmd)
                elif user_input.lower().startswith('send:'):
                    msg = user_input[5:].strip()
                    self.send_to_phone(msg)
                elif user_input.lower().startswith('ask:'):
                    question = user_input[4:].strip()
                    print(f"❓ 问题: {question}")
                    answer = self.call_deepseek_api(question)
                    print(f"💡 AI回答: {answer}")
                    self.send_to_phone(f"Answer: {answer}")
                else:
                    print("❌ 无效命令。请使用 at:, send:, ask:, test 或 quit")
                    
        except KeyboardInterrupt:
            print("\n用户终止程序")
        finally:
            self.running = False
    
    def close(self):
        """关闭连接"""
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            print("🔒 蓝牙串口已关闭")

def main():
    """主函数"""
    print("=" * 60)
    print("🔵 蓝牙直接通信程序")
    print("=" * 60)
    print("📋 功能说明：")
    print("- 直接连接HC05蓝牙模块串口")
    print("- 发送AT命令测试蓝牙状态")
    print("- 直接发送AI答案到手机")
    print("- 绕过STM32处理逻辑")
    print("=" * 60)
    
    API_KEY = "sk-946ee132a8c7408a9229d20a3065698b"
    
    try:
        # 创建蓝牙聊天器
        bt_chat = BluetoothDirectChat(API_KEY)
        
        if bt_chat.serial_port:
            print(f"\n✅ HC05蓝牙模块连接成功: {bt_chat.bt_port}")
            
            # 运行交互模式
            bt_chat.run_interactive_mode()
            
        else:
            print("\n❌ 未找到HC05蓝牙模块，程序退出")
            
    except Exception as e:
        print(f"\n❌ 程序运行失败: {e}")
    finally:
        if 'bt_chat' in locals():
            bt_chat.close()

if __name__ == "__main__":
    main() 