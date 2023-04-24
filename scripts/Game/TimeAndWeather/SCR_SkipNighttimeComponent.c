class SCR_SkipNighttimeComponentClass: ScriptComponentClass
{
};

class SCR_SkipNighttimeComponent : ScriptComponent
{
	[Attribute(defvalue: "120", uiwidget: UIWidgets.EditBox, desc: "Timeout for the daytime checker in seconds. This value gets scaled by the time multiplier (= TimeAndWeatherManagerEntity.GetDayDuration() divided by 86400)")]
	protected float m_fTimeout;
	// This component currently gets executed before SCR_TimeAndWeatherHandlerComponent, hence it cannot get the correct time multiplier if the GameMode class has a SCR_TimeAndWeatherHandlerComponent
	// Workaround: Set the initial time multiplier to the maximum possible value and the timeout is not changed until EOnInit is done
	protected float m_fTimeMultiplier = 12;
	protected TimeAndWeatherManagerEntity timeManager;
	protected bool m_bInitDone = false;
	
	override void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return;
		
		SetEventMask(owner, EntityEvent.INIT);
		owner.SetFlags(EntityFlags.ACTIVE, true);
	};
	
	override void EOnInit(IEntity owner)
	{
		// Run handler only on authority
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!rpl)
			return;
		if (rpl.Role() != RplRole.Authority)
			return;

		timeManager = TimeAndWeatherManagerEntity.Cast(owner);
		if (!timeManager)
			return;
		SkipNighttimeHandler();
		// Recall handler based on timeout scaled by the time multiplier
		GetGame().GetCallqueue().CallLater(SkipNighttimeHandler, m_fTimeout / m_fTimeMultiplier * 1000, true);
		m_bInitDone = true;
	};
	
	void SkipNighttimeHandler()
	{
		array<SCR_DayTimeInfoBase> daytimeInfo = new array<SCR_DayTimeInfoBase>;
		timeManager.GetDayTimeInfoArray(daytimeInfo, timeManager.GetYear(), timeManager.GetMonth(), timeManager.GetDay());
		float sunriseHours24 = daytimeInfo[0].GetDayTime();
		// Sunset hour is adjusted by the timeout such that the skipping happens shortly before the sunset
		float sunsetHours24 = daytimeInfo[3].GetDayTime() - m_fTimeout / 3600;
		float currHours24 = timeManager.GetTime().ToTimeOfTheDay();

		if (currHours24 < sunriseHours24 || currHours24 >= sunsetHours24)
		{	
			if (currHours24 >= sunsetHours24)
			{
				// Get the sunrise for the next day
				DateContainer newDate = GetNextDay(timeManager.GetYear(), timeManager.GetMonth(), timeManager.GetDay());
				timeManager.SetDate(newDate.m_iYears, newDate.m_iMonths, newDate.m_iDays);
				timeManager.GetDayTimeInfoArray(daytimeInfo, newDate.m_iYears, newDate.m_iMonths, newDate.m_iDays);
				sunriseHours24 = daytimeInfo[0].GetDayTime();
			};
			
			timeManager.SetTime(TimeContainer.FromTimeOfTheDay(sunriseHours24));
		};
		
		// Update timeout if time multiplier has changed
		float currTimeMultiplier = 86400 / timeManager.GetDayDuration();
		if (m_bInitDone && (currTimeMultiplier != m_fTimeMultiplier))
		{
			m_fTimeMultiplier = currTimeMultiplier;
			GetGame().GetCallqueue().Remove(SkipNighttimeHandler);
			GetGame().GetCallqueue().CallLater(SkipNighttimeHandler, m_fTimeout / m_fTimeMultiplier * 1000, true);
		}
	};
	
	DateContainer GetNextDay(int year, int month, int day)
	{
		day += 1;
		
		if (!timeManager.CheckValidDate(year, month, day))
		{
			if (month != 12)
			{
				month += 1;
			}	
			else
			{
				month = 1;
				year += 1;
			}
			day = 1;
		};
		
		return DateContainer(year, month, day);
	};
};

class DateContainer
{
	int m_iYears;
	int m_iMonths;
	int m_iDays;
	
	void DateContainer(int years = 0, int months = 0, int days = 0)
	{
		m_iYears = years;
		m_iMonths = months;
		m_iDays = days;
	}
};
