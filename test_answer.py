# test_answer.py
import serial
import time

def test_send_answer():
    """测试向STM32发送答案"""
    # 根据之前扫描结果，可能的串口
    ports_to_try = ["COM3", "COM6", "COM8"]
    
    for port in ports_to_try:
        try:
            print(f"🔍 尝试连接串口: {port}")
            ser = serial.Serial(
                port=port,
                baudrate=115200,
                bytesize=8,
                parity='N',
                stopbits=1,
                timeout=1
            )
            
            print(f"✅ {port} 连接成功！")
            
            # 发送测试答案
            test_answers = [
                "Today is sunny and warm.",
                "The weather is nice.",
                "Temperature is 25 degrees."
            ]
            
            for i, answer in enumerate(test_answers):
                print(f"\n📤 发送测试答案 {i+1}: {answer}")
                
                # 发送格式：+ANSWER:答案内容\r\n
                message = f"+ANSWER:{answer}\r\n"
                print(f"📝 发送格式: {repr(message)}")
                
                ser.write(message.encode('utf-8'))
                print("✅ 答案已发送")
                
                # 等待STM32处理
                print("⏰ 等待3秒...")
                time.sleep(3)
                
                # 检查是否有回应
                if ser.in_waiting > 0:
                    response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                    print(f"📡 STM32回应: {response}")
                
                print("-" * 50)
            
            ser.close()
            print(f"🔒 {port} 连接已关闭")
            break
            
        except Exception as e:
            print(f"❌ {port} 连接失败: {e}")
            continue
    else:
        print("❌ 所有串口连接都失败")

def main():
    """主函数"""
    print("=" * 60)
    print("🧪 STM32答案发送测试工具")
    print("=" * 60)
    print("📋 使用说明：")
    print("1. 确保STM32已连接并进入蓝牙模式")
    print("2. 运行此程序发送测试答案")
    print("3. 观察手机是否收到AI_answer消息")
    print("4. 观察STM32 LCD是否显示答案")
    print("=" * 60)
    
    input("📱 请确保STM32进入蓝牙模式，然后按回车继续...")
    
    test_send_answer()
    
    print("\n" + "=" * 60)
    print("✅ 测试完成！")
    print("💡 检查要点：")
    print("- 手机是否收到 'AI_answer：答案内容' 消息")
    print("- STM32 LCD问答区域是否显示答案")
    print("- 如果都没有显示，可能是串口选择错误")
    print("=" * 60)

if __name__ == "__main__":
    main() 