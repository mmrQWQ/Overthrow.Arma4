[ComponentEditorProps(category: "Overthrow/Components/Economy", description: "")]
class OVT_ShopComponentClass: OVT_ComponentClass
{}

enum OVT_ShopType
{
	SHOP_GENERAL,
	SHOP_DRUG,
	SHOP_CLOTHES,
	SHOP_FOOD,
	SHOP_ELECTRONIC,
	SHOP_GUNDEALER,
	SHOP_VEHICLE
}

class OVT_ShopComponent: OVT_Component
{
	[Attribute("1", UIWidgets.ComboBox, "Shop type", "", ParamEnumArray.FromEnum(OVT_ShopType) )]
	OVT_ShopType m_ShopType;
	
	ref map<RplId,int> m_aInventory;
	ref array<ref OVT_ShopInventoryItem> m_aInventoryItems;
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);	
		
		m_aInventory = new map<RplId,int>;
		m_aInventoryItems = new array<ref OVT_ShopInventoryItem>;
	}
	
	void AddToInventory(RplId id, int num)
	{
		Rpc(RpcAsk_AddToInventory, id, num);
	}
	
	void TakeFromInventory(RplId id, int num)
	{
		Rpc(RpcAsk_TakeFromInventory, id, num);
	}
	
	int GetStock(RplId id)
	{
		if(!m_aInventory.Contains(id)) return 0;
		return m_aInventory[id];
	}
	
	//RPC Methods
	override bool RplSave(ScriptBitWriter writer)
	{
		//Send JIP inventory
		writer.Write(m_aInventory.Count(), 32); 
		for(int i; i<m_aInventory.Count(); i++)
		{
			writer.WriteRplId(m_aInventory.GetKey(i));
			writer.Write(m_aInventory.GetElement(i),32);
		}
		
		return true;
	}
	
	override bool RplLoad(ScriptBitReader reader)
	{
		//Recieve JIP inventory
		int length, num;
		RplId id;
		
		if (!reader.Read(length, 32)) return false;
		for(int i; i<length; i++)
		{
			if (!reader.ReadRplId(id)) return false;				
			if (!reader.Read(num, 32)) return false;
			m_aInventory[id] = num;
		}
		return true;
	}
	
	protected void StreamInventory(RplId id)
	{
		Rpc(RpcDo_SetInventory, id, m_aInventory[id]);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetInventory(RplId id, int amount)
	{
		m_aInventory[id] = amount;		
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AddToInventory(RplId id, int num)
	{
		if(!m_aInventory.Contains(id))
		{
			m_aInventory[id] = 0;
		}		
		m_aInventory[id] = m_aInventory[id] + num;
		StreamInventory(id);
	}
	
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_TakeFromInventory(RplId id, int num)
	{
		if(!m_aInventory.Contains(id)) return;		
		m_aInventory[id] = m_aInventory[id] - num;		
		if(m_aInventory[id] < 0) m_aInventory[id] = 0;
		StreamInventory(id);
	}
	
	void ~OVT_ShopComponent()
	{
		if(m_aInventory)
		{
			m_aInventory.Clear();
			m_aInventory = null;
		}
		if(m_aInventoryItems)
		{
			m_aInventoryItems.Clear();
			m_aInventoryItems = null;
		}
	}
}