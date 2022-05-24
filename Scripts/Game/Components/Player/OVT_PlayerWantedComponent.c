[ComponentEditorProps(category: "Overthrow/Components", description: "")]
class OVT_PlayerWantedComponentClass: OVT_ComponentClass
{}

class OVT_PlayerWantedComponent: OVT_Component
{
	[Attribute("0", params: "0 5 1", desc: "The current wanted level of this character")]
	protected int m_iWantedLevel;
	protected bool m_bIsSeen = false;	
	protected int m_iWantedTimer = 0;
	protected int m_iLastSeen;
	
	protected const int LAST_SEEN_MAX = 15;	
	protected const int WANTED_TIMEOUT = 15000;
	protected const int WANTED_SYSTEM_FREQUENCY = 1000;
	
	protected FactionAffiliationComponent m_Faction;
	protected BaseWeaponManagerComponent m_Weapon;
	
	void SetWantedLevel(int level)
	{
		m_iWantedLevel = level;
	}
	
	int GetWantedLevel()
	{
		return m_iWantedLevel;
	}
	
	bool IsSeen()
	{
		return m_bIsSeen;
	}
	
	void CheckUpdate()
	{		
		m_bIsSeen = false;
		m_iLastSeen = LAST_SEEN_MAX;
		
		GetGame().GetWorld().QueryEntitiesBySphere(GetOwner().GetOrigin(), 1000, CheckEntity, FilterEntities, EQueryEntitiesFlags.ALL);
				
		Faction currentFaction = m_Faction.GetAffiliatedFaction();
		
		//Print("Last seen is: " + m_iLastSeen);
		
		if(m_iWantedLevel > 0 && !m_bIsSeen)
		{
			m_iWantedTimer -= WANTED_SYSTEM_FREQUENCY;
			//Print("Wanted timeout tick -1");
			if(m_iWantedTimer <= 0)
			{				
				m_iWantedLevel -= 1;
				Print("Downgrading wanted level to " + m_iWantedLevel);
				m_iWantedTimer = WANTED_TIMEOUT;
			}
		}
		
		
		if(m_iWantedLevel > 1 && !currentFaction)
		{
			Print("You are wanted now");
			m_Faction.SetAffiliatedFactionByKey(m_Config.m_sPlayerFaction);
		}
		
		if(m_iWantedLevel < 2 && currentFaction)
		{
			Print("You are no longer wanted");
			m_Faction.SetAffiliatedFactionByKey("");
		}
	}
	
	bool CheckEntity(IEntity entity)
	{				
		PerceptionComponent perceptComp = PerceptionComponent.Cast(entity.FindComponent(PerceptionComponent));
		if(!perceptComp) return true;
		
		BaseTarget possibleTarget = perceptComp.GetClosestTarget(ETargetCategory.FACTIONLESS, LAST_SEEN_MAX);
				
		IEntity realEnt = NULL;
		
		if (possibleTarget)
			realEnt = possibleTarget.GetTargetEntity();
		
		if (realEnt && realEnt == GetOwner())
		{
			
			int lastSeen = possibleTarget.GetTimeSinceSeen();
			
			if(lastSeen < m_iLastSeen) m_iLastSeen = lastSeen;
			if(m_iWantedLevel > 1) return false;
			
			if(TraceLOS(entity, GetOwner())){
				//Player can be seen with direct Line of sight
				m_bIsSeen = true;
								
				if (m_Weapon && m_Weapon.GetCurrentWeapon())
				{
					//Player is brandishing a weapon
					m_iWantedLevel = 2;
				}	
			}
			return false;		
		}
		
		//Continue search
		return true;
	}
	
	private bool TraceLOS(IEntity source, IEntity dest)
	{		
		int headBone = source.GetBoneIndex("head");
		vector matPos[4];
		
		source.GetBoneMatrix(headBone, matPos);
		vector headPos = source.CoordToParent(matPos[3]);
						
		autoptr TraceParam param = new TraceParam;
		param.Start = headPos;
		param.End = dest.GetOrigin();
		param.LayerMask = EPhysicsLayerDefs.Perception;
		param.Flags = TraceFlags.ENTS | TraceFlags.WORLD; 
		param.Exclude = source;
			
		float percent = GetGame().GetWorld().TraceMove(param, null);
			
		// If trace hits the target entity or travels the entire path, return true	
		GenericEntity ent = GenericEntity.Cast(param.TraceEnt);
		if (ent)
		{
			if ( ent == dest || ent.GetParent() == dest )
				return true;
		} 
		else if (percent == 1)
			return true;
				
		return false;
	}
	
	bool FilterEntities(IEntity ent) 
	{			
		
		if(ent == GetOwner())
			return false;
		
		if (ent.FindComponent(AIControlComponent)){
			FactionAffiliationComponent faction = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
				
			if(!faction) return false;
			
			Faction currentFaction = faction.GetAffiliatedFaction();
			
			if(currentFaction && currentFaction.GetFactionKey() == m_Config.m_sOccupyingFaction)
				return true;
		}
				
		return false;		
	}
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		m_iWantedTimer = WANTED_TIMEOUT;
		
		m_Faction = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
		m_Weapon = BaseWeaponManagerComponent.Cast(owner.FindComponent(BaseWeaponManagerComponent));
		
		GetGame().GetCallqueue().CallLater(CheckUpdate, WANTED_SYSTEM_FREQUENCY, true, owner);		
	}
	
	override void OnDelete(IEntity owner)
	{
		if (!Replication.IsServer())
			return;
		
		GetGame().GetCallqueue().Remove(CheckUpdate);
	}
}