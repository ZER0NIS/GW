#include "ScriptSystem.h"

ScriptSystem* ScriptSystem::m_This = NULL;

ScriptSystem::ScriptSystem()
{
}

ScriptSystem::~ScriptSystem()
{
	// Liberar todos los m�dulos almacenados
	for (map<string, PyObject*>::iterator it = m_Modules.begin(); it != m_Modules.end(); ++it) {
		if (it->second) {
			Py_DECREF(it->second);
		}
	}

	m_Modules.clear();
}

ScriptSystem* ScriptSystem::Instance()
{
	if (!m_This)
		m_This = new ScriptSystem();

	return m_This;
}

void ScriptSystem::Release()
{
	// Asegurarse de que el singleton existe antes de liberarlo
	if (m_This) {
		// Finalizar el int�rprete de Python despu�s de liberar las referencias
		Py_Finalize();

		delete m_This;
		m_This = NULL;
	}
}

void ScriptSystem::Init()
{
	Py_Initialize();
}

bool ScriptSystem::RunScript(const char* name)
{
	try
	{
		PyRun_SimpleString(name);
	}
	catch (...)
	{
		PyErr_Print();
		PyErr_Clear();

		return false;
	}

	return true;
}

bool ScriptSystem::RunScriptFile(const char* name)
{
	string str = "execfile(\'";
	str += name;
	str += "\')";

	try
	{
		PyRun_SimpleString(str.c_str());
	}
	catch (...)
	{
		PyErr_Print();
		PyErr_Clear();

		return false;
	}

	return true;
}

PyObject* ScriptSystem::ImportModule(const char* name)
{
	// Verificar si el m�dulo ya existe en nuestro mapa
	map<string, PyObject*>::iterator it = m_Modules.find(name);
	if (it != m_Modules.end()) {
		return it->second;  // Devolver el m�dulo existente
	}

	// Importar el m�dulo
	PyObject* Module = PyImport_ImportModule(name);

	if (!Module) {
		PyErr_Print();
		PyErr_Clear();
		return NULL;
	}

	// Incrementar la referencia ya que lo almacenamos en nuestro mapa
	Py_INCREF(Module);

	// Almacenar en el mapa para futuras referencias
	m_Modules[name] = Module;

	return Module;
}

PyObject* ScriptSystem::GetModule(string name)
{
	if (m_Modules.find(name) == m_Modules.end())
		return NULL;

	return m_Modules[name];
}

PyObject* ScriptSystem::Reload(const char* name)
{
	map<string, PyObject*>::iterator iter = m_Modules.find(name);

	if (iter == m_Modules.end())
		return NULL;

	// Guardar el m�dulo antiguo para liberar la referencia despu�s
	PyObject* oldModule = iter->second;

	// Recargar el m�dulo
	PyObject* newModule = PyImport_ReloadModule(oldModule);

	if (!newModule) {
		PyErr_Print();
		PyErr_Clear();
		return NULL;
	}

	// Actualizar el m�dulo en el mapa (no es necesario incrementar la referencia
	// porque PyImport_ReloadModule ya devuelve una nueva referencia)
	iter->second = newModule;

	// Liberar la referencia al m�dulo antiguo
	Py_DECREF(oldModule);

	return newModule;
}

void ScriptSystem::ReloadAll()
{
	for (map<string, PyObject*>::iterator iter = m_Modules.begin(); iter != m_Modules.end(); ++iter)
	{
		// Guardar el m�dulo antiguo
		PyObject* oldModule = iter->second;

		// Recargar el m�dulo
		PyObject* newModule = PyImport_ReloadModule(oldModule);

		if (!newModule) {
			PyErr_Print();
			PyErr_Clear();
			continue;
		}

		// Actualizar el m�dulo en el mapa
		iter->second = newModule;

		// Liberar la referencia al m�dulo antiguo
		Py_DECREF(oldModule);
	}
}