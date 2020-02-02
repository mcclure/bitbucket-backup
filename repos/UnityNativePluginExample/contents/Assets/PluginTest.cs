using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PluginTest : MonoBehaviour {

	// Use this for initialization
	void Start () {
		Debug.Log("Plugin test function returns: " + Plugin.UnityPluginCustomTest());
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}
