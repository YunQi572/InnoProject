#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DeepSeek API连接测试脚本
用于验证API密钥和网络连接是否正常
"""

from openai import OpenAI

def test_deepseek_api():
    """测试DeepSeek API连接"""
    
    # 使用硬编码的API密钥（从ai_chat_bridge.py中获取）
    API_KEY = "sk-946ee132a8c7408a9229d20a3065698b"
    BASE_URL = "https://api.deepseek.com"
    
    print("=" * 50)
    print("DeepSeek API连接测试")
    print("=" * 50)
    
    try:
        # 创建客户端
        client = OpenAI(api_key=API_KEY, base_url=BASE_URL)
        print(f"✅ 客户端创建成功")
        print(f"📡 API地址: {BASE_URL}")
        print(f"🔑 API密钥: {API_KEY[:8]}...")
        
        # 测试API调用
        print("\n🚀 正在测试API调用...")
        
        response = client.chat.completions.create(
            model="deepseek-chat",
            messages=[
                {"role": "system", "content": "你是一个智能助手，请用简洁的中文回答。"},
                {"role": "user", "content": "你好，请简单介绍一下你自己"}
            ],
            stream=False,
            max_tokens=100,
            temperature=0.7
        )
        
        answer = response.choices[0].message.content
        print("✅ API调用成功!")
        print(f"🤖 AI回答: {answer}")
        
        # 测试问答功能
        print("\n" + "=" * 30)
        print("问答功能测试")
        print("=" * 30)
        
        test_questions = [
            "今天天气怎么样？",
            "什么是人工智能？",
            "推荐一本好书"
        ]
        
        for i, question in enumerate(test_questions, 1):
            print(f"\n📝 测试问题 {i}: {question}")
            
            response = client.chat.completions.create(
                model="deepseek-chat",
                messages=[
                    {"role": "system", "content": "你是一个智能助手，请用简洁、友好的中文回答问题，回答控制在50字以内。"},
                    {"role": "user", "content": question}
                ],
                stream=False,
                max_tokens=100,
                temperature=0.7
            )
            
            answer = response.choices[0].message.content.strip()
            print(f"🤖 AI回答: {answer}")
        
        print("\n" + "=" * 50)
        print("✅ 所有测试通过！API连接正常！")
        print("🎉 可以正常使用ai_chat_bridge.py了！")
        print("=" * 50)
        
        return True
        
    except Exception as e:
        error_msg = str(e)
        print(f"\n❌ API测试失败!")
        print(f"错误信息: {error_msg}")
        
        # 详细错误分析
        if "401" in error_msg or "unauthorized" in error_msg.lower():
            print("\n🔑 可能的问题: API密钥无效")
            print("解决方案:")
            print("- 检查API密钥是否正确")
            print("- 确认在DeepSeek平台上API密钥是否有效")
            print("- 访问 https://platform.deepseek.com 检查密钥状态")
            
        elif "quota" in error_msg.lower() or "limit" in error_msg.lower():
            print("\n💰 可能的问题: API额度不足")
            print("解决方案:")
            print("- 检查DeepSeek账户余额")
            print("- 充值或等待免费额度重置")
            
        elif "timeout" in error_msg.lower():
            print("\n🌐 可能的问题: 网络超时")
            print("解决方案:")
            print("- 检查网络连接")
            print("- 尝试使用VPN")
            print("- 稍后重试")
            
        elif "connection" in error_msg.lower():
            print("\n🌐 可能的问题: 网络连接失败")
            print("解决方案:")
            print("- 检查网络连接")
            print("- 检查防火墙设置")
            print("- 尝试使用代理")
            
        else:
            print(f"\n❓ 未知错误: {error_msg}")
            print("建议:")
            print("- 检查网络连接")
            print("- 确认API密钥正确")
            print("- 查看完整错误信息")
        
        return False

if __name__ == "__main__":
    test_deepseek_api() 