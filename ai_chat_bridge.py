# ai_chat_bridge.py
import serial
import re
import time
import threading
from openai import OpenAI

class DeepSeekChatBridge:
    def __init__(self, com_port, api_key, api_base_url="https://api.deepseek.com/v1"):
        """
        初始化聊天桥接器
        :param com_port: 串口号，如 'COM3' (Windows) 或 '/dev/ttyUSB0' (Linux)
        :param api_key: DeepSeek API密钥
        :param api_base_url: API基础URL
        """
        self.serial_port = None
        self.api_key = api_key
        self.api_base_url = api_base_url
        self.com_port = com_port
        self.running = False
        
        # 初始化OpenAI客户端
        self.client = OpenAI(api_key=api_key, base_url=api_base_url)
        
        # 初始化串口
        self.init_serial()
    
    def init_serial(self):
        """初始化串口连接"""
        # 尝试主要串口
        ports_to_try = [self.com_port]
        
        # 添加备选串口
        backup_ports = ["COM11", "COM15", "COM9", "COM3"]
        for port in backup_ports:
            if port not in ports_to_try:
                ports_to_try.append(port)
        
        for port in ports_to_try:
            try:
                print(f"尝试连接串口: {port}")
                self.serial_port = serial.Serial(
                    port=port,
                    baudrate=115200,
                    bytesize=8,
                    parity='N',
                    stopbits=1,
                    timeout=1
                )
                print(f"串口 {port} 连接成功！")
                self.com_port = port  # 更新成功的串口
                return True
            except Exception as e:
                print(f"串口 {port} 连接失败: {e}")
                continue
        
        print("所有串口连接都失败，请检查：")
        print("1. STM32设备是否已连接")
        print("2. 设备管理器中的串口号")
        print("3. 是否有其他程序占用串口")
        return False
    
    def call_deepseek_api(self, question):
        """调用DeepSeek API"""
        try:
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
            error_msg = str(e)
            if "timeout" in error_msg.lower():
                return "请求超时，请检查网络连接"
            elif "connection" in error_msg.lower():
                return "网络连接失败，请检查网络"
            elif "401" in error_msg or "unauthorized" in error_msg.lower():
                return "API密钥无效，请检查密钥"
            elif "quota" in error_msg.lower() or "limit" in error_msg.lower():
                return "API额度不足，请检查账户余额"
            else:
                return f"API调用失败: {error_msg}"
    
    def extract_question(self, data):
        """从接收到的数据中提取问题"""
        # 清理数据
        data = data.strip().replace('\r', '').replace('\n', '')
        
        # 优先匹配 +QUESTION:xxx 格式
        match = re.search(r'\+QUESTION:(.+)', data)
        if match:
            question = match.group(1).strip()
            return question
        
        # 匹配包含问号的问题
        if '?' in data:
            # 移除可能的前缀命令
            question = data
            # 移除常见的前缀
            prefixes = ['+ASK:', 'ASK:', 'QUESTION:', 'Q:']
            for prefix in prefixes:
                if data.upper().startswith(prefix.upper()):
                    question = data[len(prefix):].strip()
                    break
            return question
        
        return None
    
    def send_answer(self, answer):
        """发送答案到STM32"""
        try:
            # 确保答案不超过最大长度
            if len(answer) > 400:
                original_len = len(answer)
                answer = answer[:400] + "..."
                print(f"⚠️  答案过长({original_len}字符)，已截断至400字符")
            
            # 发送格式: +ANSWER:xxx\r\n
            message = f"+ANSWER:{answer}\r\n"
            
            print(f"📤 准备发送答案到STM32:")
            print(f"   📊 答案长度: {len(answer)} 字符")
            print(f"   📝 答案内容: '{answer}'")
            print(f"   🔢 发送格式: {repr(message)}")
            print(f"   📡 串口: {self.com_port}")
            
            # 发送数据
            bytes_sent = self.serial_port.write(message.encode('utf-8'))
            self.serial_port.flush()  # 强制刷新串口缓冲区
            
            print(f"✅ 发送完成: {bytes_sent} 字节")
            print(f"💡 请检查:")
            print(f"   - STM32 LCD问答区域是否显示答案")
            print(f"   - 手机是否收到 'AI_answer：{answer}' 消息")
            
            return True
        except Exception as e:
            print(f"❌ 发送答案失败: {e}")
            return False
    
    def process_question_async(self, question):
        """异步处理问题"""
        def process():
            # 调用DeepSeek API
            answer = self.call_deepseek_api(question)
            
            # 突出显示AI回答
            print("-"*60)
            print("✅ AI回答完成！")
            print("-"*60)
            print(f"💡 回答: {answer}")
            print("-"*60)
            print(f"📤 正在发送到STM32...")
            
            # 发送答案
            if self.send_answer(answer):
                print("✅ 答案发送成功！\n")
            else:
                print("❌ 答案发送失败！\n")
        
        # 在新线程中处理，避免阻塞主循环
        thread = threading.Thread(target=process)
        thread.daemon = True
        thread.start()
    
    def run(self):
        """主运行循环"""
        self.running = True
        print("=" * 60)
        print("🚀 AI聊天桥接器已启动")
        print("=" * 60)
        print("📡 等待STM32发送问题...")
        print("\n📋 使用方法：")
        print("1. 确保STM32已连接并进入蓝牙模式")
        print("2. 手机通过蓝牙发送包含'?'的问题")
        print("3. 程序会在终端显示接收到的问题")
        print("4. AI会自动回答并发送回STM32")
        # print("\n🧪 测试模式：在终端输入 'test:你的问题?' 进行测试")
        print("⌨️  按 Ctrl+C 退出程序")
        print("=" * 60)
        
        # 启动测试输入线程
        def test_input_thread():
            while self.running:
                try:
                    user_input = input()
                    if user_input.lower().startswith('test:'):
                        test_question = user_input[5:].strip()
                        print(f"\n🧪 测试模式 - 模拟接收: {test_question}")
                        question = self.extract_question(test_question)
                        if question:
                            print(f"✅ 问题提取成功: {question}")
                            self.process_question_async(question)
                        else:
                            print(f"❌ 问题提取失败，请包含'?'符号")
                except:
                    break
        
        test_thread = threading.Thread(target=test_input_thread)
        test_thread.daemon = True
        test_thread.start()
        
        try:
            buffer = ""
            data_count = 0
            while self.running:
                if self.serial_port and self.serial_port.in_waiting > 0:
                    # 读取串口数据
                    try:
                        raw_data = self.serial_port.read(self.serial_port.in_waiting)
                        data_count += 1
                        
                        # 显示原始数据（调试用）
                        print(f"\n🔍 接收到原始数据 #{data_count}:")
                        print(f"   📊 字节数: {len(raw_data)}")
                        print(f"   🔢 十六进制: {raw_data.hex(' ')}")
                        
                        # 尝试解码
                        try:
                            data = raw_data.decode('utf-8')
                            print(f"   📝 UTF-8解码: '{data}'")
                        except:
                            data = raw_data.decode('utf-8', errors='ignore')
                            print(f"   ⚠️  部分解码: '{data}'")
                        
                        buffer += data
                        print(f"   📦 当前缓冲区: '{buffer}'")
                        
                        # 按行处理数据
                        lines = buffer.split('\n')
                        buffer = lines[-1]  # 保留未完整的行
                        
                        print(f"   📋 分割后行数: {len(lines)-1}")
                        for i, line in enumerate(lines[:-1]):
                            line = line.strip()
                            print(f"   📄 第{i+1}行: '{line}'")
                            if line:
                                print(f"✅ 处理数据行: {line}")
                                
                                # 提取问题
                                question = self.extract_question(line)
                                print(f"🔍 问题提取结果: {question}")
                                
                                if question:
                                    # 突出显示接收到的问题
                                    print("\n" + "="*60)
                                    print("📝 新问题接收！")
                                    print("="*60)
                                    print(f"⏰ 时间: {time.strftime('%Y-%m-%d %H:%M:%S')}")
                                    print(f"❓ 问题: {question}")
                                    print("="*60)
                                    print("🤖 正在调用AI处理中...")
                                    
                                    # 异步处理问题
                                    self.process_question_async(question)
                                else:
                                    # 检查是否是其他格式的问题
                                    if '?' in line:
                                        print(f"⚠️  发现问号但格式不匹配: '{line}'")
                                        print("   💡 期望格式: +QUESTION:你的问题")
                        
                    except Exception as e:
                        print(f"❌ 数据处理错误: {e}")
                        continue
                
                time.sleep(0.1)  # 避免CPU占用过高
                
        except KeyboardInterrupt:
            print("\n用户终止程序")
        except Exception as e:
            print(f"运行错误: {e}")
        finally:
            self.stop()
    
    def stop(self):
        """停止程序"""
        self.running = False
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            print("串口已关闭")
        print("程序已退出")

def main():
    """主函数"""
    print("AI聊天桥接器配置")
    print("请确保已安装所需库：pip install pyserial openai")
    print()
    
    # 配置参数 - 尝试STM32常用串口
    COM_PORT = "COM3"  # 根据扫描结果，COM3可以连接
    # 备选串口列表
    backup_ports = ["COM3", "COM6", "COM8", "COM11", "COM15", "COM9"]
    
    print(f"尝试连接串口: {COM_PORT}")
    print(f"备选串口: {backup_ports}")
    
    print("\n" + "⚠️ " * 20)
    print("📱 重要提醒：请确保STM32已进入蓝牙模式！")
    print("   1. STM32开机后按KEY0进入蓝牙模式")
    print("   2. LCD显示蓝牙信息界面")
    print("   3. 手机已通过蓝牙连接到STM32")
    print("⚠️ " * 20)
    
    input("\n✅ 确认STM32已进入蓝牙模式后，按回车继续...")
    
    API_KEY = "sk-946ee132a8c7408a9229d20a3065698b"
    if not API_KEY:
        print("错误：必须提供API密钥")
        print("请访问 https://platform.deepseek.com 获取API密钥")
        return
    
    # 创建并运行桥接器
    try:
        print("\n正在初始化AI聊天桥接器...")
        bridge = DeepSeekChatBridge(COM_PORT, API_KEY)
        if bridge.serial_port and bridge.serial_port.is_open:
            print(f"\n✓ 串口连接成功: {bridge.com_port}")
            print("✓ API配置完成")
            bridge.run()
        else:
            print("\n❌ 串口初始化失败")
            print("\n解决方案：")
            print("1. 确保STM32开发板已通过USB连接到电脑")
            print("2. 检查设备管理器中的串口号")
            print("3. 确保没有其他程序（如串口调试工具）占用串口")
            print("4. 尝试重新拔插USB线")
            print("\n程序退出")
    except Exception as e:
        print(f"\n❌ 程序启动失败: {e}")
        print("请检查依赖库是否正确安装：pip install pyserial openai")

if __name__ == "__main__":
    main() 