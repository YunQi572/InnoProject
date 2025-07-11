# port_scanner.py
import serial
import time
import threading

def test_port(port_name):
    """测试指定串口是否有数据"""
    try:
        print(f"🔍 测试串口: {port_name}")
        ser = serial.Serial(
            port=port_name,
            baudrate=115200,
            bytesize=8,
            parity='N',
            stopbits=1,
            timeout=1
        )
        
        print(f"✅ {port_name} 连接成功，监听5秒...")
        
        start_time = time.time()
        while time.time() - start_time < 5:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                print(f"📡 {port_name} 接收到数据:")
                print(f"   📊 字节数: {len(data)}")
                print(f"   🔢 十六进制: {data.hex(' ')}")
                try:
                    text = data.decode('utf-8', errors='ignore')
                    print(f"   📝 文本: '{text}'")
                except:
                    print(f"   ❌ 解码失败")
                print("-" * 40)
            time.sleep(0.1)
        
        ser.close()
        print(f"⏹️  {port_name} 测试完成\n")
        
    except Exception as e:
        print(f"❌ {port_name} 连接失败: {e}\n")

def main():
    """主函数"""
    print("=" * 60)
    print("🔍 STM32串口扫描工具")
    print("=" * 60)
    print("📋 说明：")
    print("1. 确保STM32已连接并运行")
    print("2. 通过蓝牙发送问题（如'Weather?'）")
    print("3. 观察哪个串口接收到数据")
    print("=" * 60)
    
    # 要测试的串口列表
    ports_to_test = ["COM3", "COM4", "COM5", "COM6", "COM7", "COM8"]
    
    print(f"🚀 开始扫描串口: {ports_to_test}")
    print("⏰ 每个串口测试5秒...")
    print()
    
    for port in ports_to_test:
        test_port(port)
    
    print("=" * 60)
    print("✅ 扫描完成！")
    print("💡 提示：如果某个串口接收到'+QUESTION:'格式的数据，")
    print("   那就是STM32 USART1对应的串口")
    print("=" * 60)

if __name__ == "__main__":
    main() 