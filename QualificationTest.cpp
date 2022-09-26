#pragma once
#include <iostream>
#include <functional>
#include <thread>

using std::cout;
using std::endl;

void fA()
{
	std::cout << "A\n";
};
void fB()
{
	std::cout << "B\n";
};
void fC()
{
	std::cout << "C\n";
};
void fD()
{
	std::cout << "D\n";
};
void fE()
{
	std::cout << "E\n";
};

struct Links
{
	size_t node_id;
	std::vector<Links> links_vector;
	Links* owner{};
};
struct Target {
	explicit Target(std::function<void()> t) :task(t), id(++Count)
	{
	}
	Target(const Target&) = delete;
	void operator=(const Target&) = delete;
	~Target()
	{
	}
	size_t GetId()
	{
		return id;
	}
	std::function<void()> task;
	static size_t Count;
private:
	size_t id;
};
size_t Target::Count;
/// <summary>
/// /
/// </summary>
class BuildGraph
{
public:
	BuildGraph()
	{
	}
	BuildGraph(const BuildGraph&) = delete;
	void operator=(const BuildGraph&) = delete;
	~BuildGraph()
	{
	}
	void AddTarget(Target* t)
	{
		targets.push_back(t);
	}
	void DeleteTarget(Target* t)
	{
		auto res = std::find(targets.begin(), targets.end(), t);
		if (res != end(targets)) targets.erase(res);
	}
	void AddLink(size_t root_id, size_t target_id)
	{
		Links link_point{target_id ,{}};
		auto& vtarget = FindNode(links, target_id);
		// если target_id уже есть не в корне дерева, то копируем target_id и все его ветви в ветвь root_id
		if (vtarget.node_id)
		{
			link_point = { target_id, vtarget.links_vector };
		}
		// если target_id уже есть в корне дерева, то удаляем копию из корня
		if (vtarget.node_id&& !vtarget.owner->node_id)
		{
			auto res = std::find_if(links.links_vector.begin(), links.links_vector.end(),
				[target_id](Links& l)->bool {return l.node_id == target_id; });
			if (res != end(links.links_vector)) links.links_vector.erase(res);
		}
		auto &vroot=FindNode(links, root_id);
		if (!vroot.node_id)
		{
			Links link_root;
			link_root.node_id = root_id; link_root.links_vector.push_back(link_point);
			link_root.owner = &links;
			links.links_vector.push_back(link_root); 
		}
		else 
		{
			link_point.owner = &vroot;
			vroot.links_vector.push_back(link_point);
		}
	}
	void DeleteLinks()
	{
		links.links_vector.clear(); links.node_id = 0;
	}
	void DeleteLink(size_t target_id)
	{
		auto& vtarget = FindNode(links, target_id);
		if (vtarget.owner)
		{
			auto& v = vtarget.owner->links_vector;
			auto res = std::find_if(v.begin(), v.end(),
				[target_id](Links& l)->bool {return l.node_id == target_id; });
			if (res != end(v)) v.erase(res);
		}
	}

	Links& FindNode(Links& l, size_t node_id)
	{
		Links result{};
		auto &v = l.links_vector;
		auto FN = std::find_if(v.begin(), v.end(),[node_id](Links& l)->bool {return l.node_id == node_id;});
		if (FN != end(v)) return *FN;
		for (auto& elem : v)	
		{
			auto & res=FindNode(elem, node_id);
			if ( ) return res;
		}
		return result;
	}
	 size_t size()const
	{
		return targets.size();
	}
	Target* GetTargetPtr(const size_t t_id) const
	{
		for (auto& elem : targets)
		{
			if (elem->GetId() == t_id) return elem;
		}
		return nullptr;
	}
	void printTree(const Links& l= BuildGraph::rootlink)
	{
		if (l.node_id== BuildGraph::rootlink.node_id)  printTree(links);
		else {
			cout << l.node_id <<"  size = "<< l.links_vector.size() << endl;
			for (auto& elem : l.links_vector)
				cout << elem.node_id << "   ";
			cout << endl << endl;
			for (auto& elem : l.links_vector)
				printTree(elem);
		}
	}
	Links GetLinks() const
	{
		return links;
	}
	static const Links rootlink;
	std::vector<Target*> targets;
	Links links{};
};
const Links BuildGraph::rootlink{-1, {} };

class Builder {
public:
	explicit Builder(size_t num_threads): num_threads(num_threads)
	{
	}
	void execute(const BuildGraph& build_graph, size_t target_id)
	{
		size_t threadcount = num_threads;
		Links bgl= build_graph.GetLinks();
		std::vector<size_t> tasks_id;


		while (bgl.links_vector.size())
		{
			while (threadcount)
			{
				
			}
		}
	}
	bool MakeTaskOrder(const BuildGraph& build_graph,size_t target_id)
	{
//
//
//
		return true;
	}
	bool AddTask(std::vector <size_t> sublinks)
	{

	}
private:
	size_t num_threads;
	std::vector <std::vector <std::function<void()>>> tasks;
};


int main()
{
	// targ1 ->targ2 ->targ3
	//      | 
	//		 ->targ4 ->targ5
 
	Target targ1(fA), targ2(fB), targ3(fC), targ4(fD), targ5(fE);
	BuildGraph bg;
	bg.AddTarget(&targ1);
	bg.AddTarget(&targ2);
	bg.AddTarget(&targ3);
	bg.AddTarget(&targ4);
	bg.AddTarget(&targ5);
	bg.AddLink(targ2.GetId(), targ3.GetId()); bg.AddLink(targ4.GetId(), targ5.GetId());
	bg.AddLink(targ1.GetId(), targ2.GetId()); bg.AddLink(targ1.GetId(), targ4.GetId());

	
	bg.printTree();
	bg.DeleteLink(2);
	cout << "delete node 2" << endl;
	bg.printTree();
	cout << endl;

	system("pause");
}
