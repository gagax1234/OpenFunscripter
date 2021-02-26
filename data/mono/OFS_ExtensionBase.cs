using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace OFS
{
	/*
		Base API for reference.
		Do not edit.
	*/
	public class ScriptAction
	{
		public int at;
		public int pos;
		public bool selected;
	}

	public class Script
	{
		public string Name = "";
		public List<ScriptAction> actions = new List<ScriptAction>();
	}

	public class Context
	{
		public List<Script> Scripts = new List<Script>();
	}

	public abstract class Extension
	{
		public Extension() {}
		
		protected Context Ctx {get; private set;}
		
		public abstract void Init();
		public abstract void Run();
		
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private extern static Context GetContext(); // calls into ofs to get the context
		
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private extern static void UpdateFromContext(Context ctx); // loads script data back into ofs
		
		// OFS invokes this using an extension
		private static void RunExtension(Extension ext)
		{
			ext.Ctx = GetContext();
			ext.Init();
			ext.Run();
			UpdateFromContext(ext.Ctx);
		}
	}
}
