// C++ 20
#pragma once
#include <iostream>
#include <functional>
#include <mutex>
#include <thread>
#include <set>
//#include <chrono>

using std::cout;
using std::endl;
//using namespace std::chrono_literals;

std::mutex m;
struct Target;
struct Links;
class Task;
class BuildGraph;
class Builder;

class Task// задачи для теста работы
{
public:
	Task() :id(++count) {}
	void task()
	{
		std::string s = "Task " + std::to_string(id) + " completed!\n";
		std::cout << s;
	}
private:
	static size_t count;
	size_t id;
};
size_t Task::count;

struct Target
{
	// обертка для выполняемой в потоке задачи
	struct ThreadWrap
	{
		ThreadWrap(std::function<void()> t, Target* owner) :task(t), owner(owner) {}
		// деструктор используется как сигнал окончания работы задачи и запускает связанный метод другого класса
		~ThreadWrap()
		{
			owner->completed = true;
			if (owner->taskcompleted)
				(owner->builder->*(owner->taskcompleted))(owner->id);
		}
		void operator ()()
		{
			task();
		}
		std::function<void()> task;
		Target* owner;
	};
	explicit Target(std::function<void()> t) :task(t), id(++count)
	{}
	size_t GetId()
	{
		return id;
	}
	// установка указателей на метод класса Builder, который выолнится после завершения задачи task
	void SetCompleteTask(Builder* b, void (Builder::* bf) (size_t))
	{
		builder = b;
		taskcompleted = bf;
	}
	// асинхронный запуск задачи в обертке
	void StartTask()
	{
		std::thread(*new ThreadWrap(task, this)).detach();
	}
	volatile bool working{};
	volatile bool completed{};
	std::list <Target*> children;
	std::list <Target*> parents;
private:
	static size_t count;
	size_t id;
	std::function<void()> task;
	Builder* builder{};
	void (Builder::* taskcompleted) (size_t) {};
};
size_t Target::count;

class BuildGraph
{
public:
	friend Builder;
	void AddTarget(Target* t)
	{
		targets.push_back(t);
	}
	void DeleteTarget(Target* t)
	{
		auto res = std::find(targets.begin(), targets.end(), t);
		if (res != end(targets)) targets.erase(res);
	}
	Target* TargetFromId(size_t id) const
	{
		auto& v = targets;
		auto res = find_if(v.begin(), v.end(), [id](Target* t)->bool
			{return t->GetId() == id; });
		if (res != end(v)) return *res;
		return nullptr;
	}
	void AddLink(size_t root_id, size_t branch_id)
	{
		Target* RootTarget, * BranchTarget;
		RootTarget = TargetFromId(root_id);
		BranchTarget = TargetFromId(branch_id);
		auto& rchildren = RootTarget->children;
		auto& bparents = BranchTarget->parents;

		auto fch = find_if(rchildren.begin(), rchildren.end(), [branch_id](Target* t)->bool {return t->GetId() == branch_id; });
		if (fch == end(rchildren)) rchildren.push_back(BranchTarget);

		auto bp = find_if(bparents.begin(), bparents.end(), [root_id](Target* t)->bool {return t->GetId() == root_id; });
		if (bp == end(bparents)) bparents.push_back(RootTarget);
	}
	void DeleteLink(size_t root_id, size_t branch_id)
	{
		Target* RootTarget, * BranchTarget;
		RootTarget = TargetFromId(root_id);
		BranchTarget = TargetFromId(branch_id);
		auto& rchildren = RootTarget->children;
		auto& bparents = BranchTarget->parents;
		auto fch = find_if(rchildren.begin(), rchildren.end(), [branch_id](Target* t)->bool {return t->GetId() == branch_id; });
		if (fch != end(rchildren)) rchildren.erase(fch);

		auto bp = find_if(bparents.begin(), bparents.end(), [root_id](Target* t)->bool {return t->GetId() == root_id; });
		if (bp != end(bparents)) bparents.erase(bp);
	}
	void DeleteAllLinksToTarget(size_t id)
	{
		for (auto& elem : targets)
		{
			auto& v = elem->children;
			auto res = find_if(v.begin(), v.end(), [id](Target* t)->bool { return t->GetId() == id; });
			if (res != end(v)) v.erase(res);
		}
	}
	void DeleteCompletedFlags()
	{
		for (auto& t : targets)
		{
			t->working = t->completed = false;
		}
	}
private:
	std::vector<Target*> targets;
};
class Builder
{
public:
	explicit Builder(size_t num_threads) : MAX_threads(num_threads)
	{
	}
	void execute(const BuildGraph& build_graph, size_t target_id)
	{
		bg = &build_graph;
		links.clear(); tasks_id.clear();
		Target* RootTarget = bg->TargetFromId(target_id);
		if (Verifying(target_id)) { cout << "Links OK!\n"; }
		else { cout << "Links Error!\n"; throw std::exception("Builder::exeute can not make a proper task queue"); };

		for (auto& v : links)
		{
			for (auto& l : v)
				cout << l << " ";
			cout << "\n";
		}

		while (links.size())
		{
			MakeTaskQueue();
			m.lock();
			for (auto& elem : tasks_id)
			{
				if (elem)
				{
					Target* task = bg->TargetFromId(elem);
					if (task && !task->working)
					{
						task->working = true;
						task->SetCompleteTask(this, &Builder::TaskCompleted);
						task->StartTask();// запуск задачи в отдельном потоке
					}
				}
			}
			m.unlock();
			while (tasks_id.size() == MAX_threads)
			{
			}
		}
	}
private:
	void TaskCompleted(size_t id)// 
	{
		std::lock_guard <std::mutex> lock(m);
		for (auto& v : links)
		{
			auto res = find(v.begin(), v.end(), id);// удаляем id выполненой задачи из всех списков вектора
			if (res != end(v)) v.erase(res);
		}
		auto res2 = find_if(links.begin(), links.end(), [id](std::list <size_t>& l)->bool {return !l.size(); });
		while (res2 != end(links))
		{
			links.erase(res2);// удаляем из вектора пустые списки
			res2 = find_if(links.begin(), links.end(), [id](std::list <size_t>& l)->bool {return !l.size(); });
		}

		auto res3 = find(tasks_id.begin(), tasks_id.end(), id);// из вектора id задач удаляем id выполненой задачи
		if (res3 != end(tasks_id)) tasks_id.erase(res3);
	}
	// создаем набор задач для отправки на выполнение в потоках
	void MakeTaskQueue()
	{
		std::list <size_t> completed_task_ids;
		{
			std::lock_guard <std::mutex> lock(m);

			for (auto& v : links)
			{
				if (tasks_id.size() == MAX_threads) return;
				size_t target_id = v.back();
				Target* t = bg->TargetFromId(target_id);
				if (t->completed) completed_task_ids.push_back(target_id);
				if (!t->working && !t->completed)
				{
					if (CheckAllBranch(target_id)) tasks_id.insert(target_id);
				}
			}
		}
		// удаление из всего вектора списка id тех задач, которые имеют статус completed
		for (auto& elem : completed_task_ids) TaskCompleted(elem);
	}
	// проверяем, что данный id или отсутсвует в других ветках или является крайним и не требует выполнения предыдущей задачи
	bool CheckAllBranch(size_t id)
	{
		bool found{};
		for (auto& v : links)
		{
			auto res = find(v.begin(), v.end(), id);
			if (res != end(v) && res != prev(v.end(), 1)) found = true;
		}
		return !found;
	}
	bool Verifying(size_t id)
	{
		std::list <size_t> chain;
		Target* rootTarget = bg->TargetFromId(id);
		if (!rootTarget->children.size()) { links.push_back({ rootTarget->GetId()}); return true; }
		// zeroRoot нужен для того, чтобы chainLinksError мог рекурсивно пройтись по всем потомкам включая rootTarget
		Target zeroRoot(nullptr);
		zeroRoot.children.push_back(rootTarget);
		bool result{ false };
		chainLinksError(&zeroRoot, chain, result);// создает вектор параллельных списков задач
		return !result;
	}
	// если в последовательности задач одна задача повторяется, значит возникает петля приводящая к ошибке
	void chainLinksError(Target* t, std::list <size_t>& chain, bool& result)
	{
		for (auto& elem : t->children)
		{
			std::list <size_t> tmpchain = chain;
			tmpchain.push_back(elem->GetId());
			if (elem->children.size() && !result) { chainLinksError(elem, tmpchain, result); }
			else
			{
				links.push_back(tmpchain);
				tmpchain.sort();
				auto res = std::adjacent_find(tmpchain.begin(), tmpchain.end());
				if (res != end(tmpchain)) { result = true;return; }
			}
		}
	}
private:
	std::set<size_t> tasks_id;// набор id задач передающихся в потоки. 
	size_t MAX_threads;// макскимальный размер предыдущего набора равен количеству заданных потоков
	std::vector <std::list<size_t>> links;// вектор списков порядка выполнения задач 
	const BuildGraph* bg{};
};

int main()
{
	const size_t size = 12;
	std::unique_ptr<Target> TargetMass[size]; // Для создания обектов Target в цикле
	std::unique_ptr<Task> TaskMass[size];     // Для создания тестовых функций в цикле 
	std::function<void()>  functionMass[size];// Для приведения тестовых функций к std::function<void()> 

	for (int i{}; i < size; ++i)
		TaskMass[i] = std::make_unique <Task>();
	for (int i{}; i < size; ++i)
		functionMass[i] = std::bind(&Task::task, *TaskMass[i].get());
	for (int i{}; i < size; ++i)
		TargetMass[i] = std::make_unique <Target>(functionMass[i]);
	BuildGraph bg;
	for (int i{}; i < size; ++i)
		bg.AddTarget(TargetMass[i].get());
	// связывание обектов Target
	//-------------------------------------------------------------------------
	// targ1=TargetMass[0]...targN=TargetMass[N-1]
	// targ1 ->targ2 ->targ3
	//      | 
	//		 ->targ4 ->targ5 ->targ3
	//      | 
	//       ->targ6 ->targ8 ->targ9 ->targ10
	//      |       |
	//      |        ->targ2 ->targ3
	//      |       |
	//      |        ->targ11 ->targ3
	//      |       |
	//      |        ->targ12
	//      | 
	//       ->targ7
	bg.AddLink(TargetMass[1].get()->GetId(), TargetMass[2].get()->GetId());//targ2 ->targ3
	bg.AddLink(TargetMass[3].get()->GetId(), TargetMass[4].get()->GetId());//targ4 ->targ5
	bg.AddLink(TargetMass[0].get()->GetId(), TargetMass[1].get()->GetId());//targ1 ->targ2
	bg.AddLink(TargetMass[0].get()->GetId(), TargetMass[3].get()->GetId());//targ1 ->targ4
	bg.AddLink(TargetMass[0].get()->GetId(), TargetMass[5].get()->GetId());//targ1 ->targ6
	bg.AddLink(TargetMass[0].get()->GetId(), TargetMass[6].get()->GetId());//targ1 ->targ7
	bg.AddLink(TargetMass[5].get()->GetId(), TargetMass[7].get()->GetId());//targ6 ->targ8
	bg.AddLink(TargetMass[5].get()->GetId(), TargetMass[7].get()->GetId());//targ6 ->targ8   // повторное указание для теста стабильности
	bg.AddLink(TargetMass[5].get()->GetId(), TargetMass[1].get()->GetId());//targ6 ->targ2   // дублирование задач в разных ветках
	bg.AddLink(TargetMass[10].get()->GetId(), TargetMass[2].get()->GetId());//targ11->targ3  // дублирование задач в разных ветках
	bg.AddLink(TargetMass[4].get()->GetId(), TargetMass[2].get()->GetId());//targ5 ->targ3   // дублирование задач в разных ветках
	//bg.AddLink(TargetMass[5].get()->GetId(), TargetMass[0].get()->GetId());//targ6 ->targ1 // !!!!зацикливание вызывает ошибку
	bg.AddLink(TargetMass[7].get()->GetId(), TargetMass[8].get()->GetId());//targ8 ->targ9
	bg.AddLink(TargetMass[8].get()->GetId(), TargetMass[9].get()->GetId());//targ9 ->targ10
	bg.AddLink(TargetMass[5].get()->GetId(), TargetMass[10].get()->GetId());//targ6 ->targ11
	bg.AddLink(TargetMass[5].get()->GetId(), TargetMass[11].get()->GetId());//targ6 ->targ12
	//-------------------------------------------------------------------------

	Builder builder(2);
	try { builder.execute(bg, 1); }
	catch (...) {};
	cout << endl << endl;

	bg.DeleteCompletedFlags();// обнуляет флаги выполненых задач 
	try { builder.execute(bg, 2); }
	catch (...) {};
	cout << endl << endl;

	bg.DeleteCompletedFlags();// обнуляет флаги выполненых задач 
	try { builder.execute(bg, 3); }
	catch (...) {};	
	cout << endl << endl;

	//bg.DeleteCompletedFlags();// НЕ обнуляет флаги выполненых задач
	try { builder.execute(bg, 6); }
	catch (...) {};


	cout << endl;

	system("pause");
}
