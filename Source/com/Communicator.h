#ifndef __COM_PROCESSLAUNCH_H
#define __COM_PROCESSLAUNCH_H

#include "Module.h"
#include "Administrator.h"
#include "ITracing.h"

#include "../tracing/TraceUnit.h"

namespace WPEFramework {
namespace RPC {

    class EXTERNAL Object {
    public:
        Object () 
            : _locator()
            , _className()
            , _interface(~0)
            , _version(~0)
            , _user()
            , _group() {
        }
        Object (const Object& copy) 
            : _locator(copy._locator)
            , _className(copy._className)
            , _interface(copy._interface)
            , _version(copy._version)
            , _user(copy._user)
            , _group(copy._group) {
        }
        Object (const string& locator, const string& className, const uint32_t interface, const uint32_t version, const string& user, const string& group) 
            : _locator(locator)
            , _className(className)
            , _interface(interface)
            , _version(version)
            , _user(user)
            , _group(group) {
        }
        ~Object() {
        }

        Object& operator= (const Object& RHS) {
            _locator = RHS._locator;
            _className = RHS._className;
            _interface = RHS._interface;
            _version = RHS._version;
            _user = RHS._user;
            _group = RHS._group;

            return (*this);
        }

    public:
        inline const string& Locator() const {
            return (_locator);
        }
        inline const string& ClassName() const {
            return (_className);
        }
        inline uint32_t Interface() const {
            return (_interface);
        }
        inline uint32_t Version() const {
            return (_version);
        }
        inline const string& User() const {
            return (_user);
        }
        inline const string& Group() const {
            return (_group);
        }

    private:
        string _locator;
        string _className;
        uint32_t _interface;
        uint32_t _version;
        string _user;
        string _group;
 
    };

    class EXTERNAL Config {
    private:
        Config& operator=(const Config&);

    public:
        Config()
            : _connector()
            , _hostApplication()
            , _persistent()
            , _system()
            , _data()
            , _application()
            , _proxyStub() {
        }
        Config(
            const string& connector, 
            const string& hostApplication, 
            const string& persistentPath,
            const string& systemPath,
            const string& dataPath,
            const string& applicationPath,
            const string& proxyStubPath) 
            : _connector(connector)
            , _hostApplication(hostApplication)
            , _persistent(persistentPath)
            , _system(systemPath)
            , _data(dataPath)
            , _application(applicationPath)
            , _proxyStub(proxyStubPath) {
        }
        Config(const Config& copy)
            : _connector(copy._connector)
            , _hostApplication(copy._hostApplication)
            , _persistent(copy._persistent)
            , _system(copy._system)
            , _data(copy._data)
            , _application(copy._application)
            , _proxyStub(copy._proxyStub) {
        }
        ~Config() {
        }

    public: 
        inline const string& Connector() const {
            return (_connector);
        }
        inline const string& HostApplication() const {
            return (_hostApplication);
        }
        inline const string& PersistentPath() const {
            return (_persistent);
        }
        inline const string& SystemPath() const {
            return (_system);
        }
        inline const string& DataPath() const {
            return (_data);
        }
        inline const string& ApplicationPath() const {
            return (_application);
        }
        inline const string& ProxyStubPath() const {
            return (_proxyStub);
        }

    private:
        string _connector;	
        string _hostApplication;
        string _persistent;
        string _system;
        string _data;
        string _application;
        string _proxyStub;
    };

    struct EXTERNAL IRemoteProcess : virtual public Core::IUnknown {
        enum { ID = 0x00000001 };

        virtual ~IRemoteProcess() {}

        struct INotification : virtual public Core::IUnknown {
            enum { ID = 0x00000002 };

            virtual ~INotification() {}
			virtual void Activated(IRemoteProcess* process) = 0;
			virtual void Deactivated(IRemoteProcess* process) = 0;
        };

        enum enumState {
            CONSTRUCTED,
            ACTIVE,
            DEACTIVATED
        };

        virtual uint32_t Id() const = 0;
        virtual enumState State() const = 0;
        virtual void* Instantiate(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version) = 0;
        virtual void Terminate() = 0;

        template <typename REQUESTEDINTERFACE>
        REQUESTEDINTERFACE* Instantiate(const uint32_t waitTime, const string& className, const uint32_t version)
        {
            void* baseInterface(Instantiate(waitTime, className, REQUESTEDINTERFACE::ID, version));

            if (baseInterface != nullptr) {
                return (reinterpret_cast<REQUESTEDINTERFACE*>(baseInterface));
            }

            return (nullptr);
        }
    };

    class EXTERNAL Communicator {

    private:
        class RemoteProcessMap;

    public:
        class EXTERNAL RemoteProcess : public IRemoteProcess {
        private:
            friend class Core::Service<RemoteProcess>;
            friend class RemoteProcessMap;

            RemoteProcess() = delete;
            RemoteProcess(const RemoteProcess&) = delete;
            RemoteProcess& operator=(const RemoteProcess&) = delete;

			RemoteProcess(RemoteProcessMap* parent, uint32_t pid, const Core::ProxyType<Core::IPCChannel>& channel)
				: _parent(parent)
				, _process(pid)
				, _state(IRemoteProcess::ACTIVE)
				, _channel(channel)
				, _returnedInterface(nullptr)
			{
			}
			RemoteProcess(RemoteProcessMap* parent, uint32_t* pid, const Core::Process::Options* options)
                : _parent(parent)
                , _process(false)
                , _state(IRemoteProcess::CONSTRUCTED)
                , _channel()
                , _returnedInterface(nullptr)
            {
                // Start the external process launch..
                _process.Launch(*options, pid);
            }

        public:
            inline static RemoteProcess* Create(RemoteProcessMap& parent, const uint32_t pid, Core::ProxyType<Core::IPCChannel>& channel)
            {
				RemoteProcess* result (Core::Service<RemoteProcess>::Create<RemoteProcess>(&parent, pid, channel));

				ASSERT(result != nullptr);

				// As the channel is there, Announde came in over it :-), we are active by default.
				parent.Activated(result);

				return (result);
            }
	inline static RemoteProcess* Create(RemoteProcessMap& parent, uint32_t& pid, const Object& instance, const Config& config)
	{
		Core::Process::Options options(config.HostApplication());
 
                ASSERT(instance.Locator().empty() == false);
                ASSERT(instance.ClassName().empty() == false);
                ASSERT(config.Connector().empty() == false);

		options[_T("l")] = instance.Locator();
		options[_T("c")] = instance.ClassName();
		options[_T("r")] = config.Connector();
		options[_T("i")] = Core::NumberType<uint32_t>(instance.Interface()).Text();
		if (instance.Version() != static_cast<uint32_t>(~0)) {
			options[_T("v")] = Core::NumberType<uint32_t>(instance.Version()).Text();
		}
		if (instance.User().empty() == false) {
			options[_T("u")] = instance.User();
                }
                if (instance.Group().empty() == false) {
			options[_T("g")] = instance.Group();
                }
		if (config.PersistentPath().empty() == false) {
		    options[_T("p")] = config.PersistentPath();
		}
	        if (config.SystemPath().empty() == false) {
		    options[_T("s")] = config.SystemPath();
		}
	        if (config.DataPath().empty() == false) {
		    options[_T("d")] = config.DataPath();
		}
	        if (config.ApplicationPath().empty() == false) {
		    options[_T("a")] = config.ApplicationPath();
		}
	        if (config.ProxyStubPath().empty() == false) {
		    options[_T("m")] = config.ProxyStubPath();
		}

		return (Core::Service<RemoteProcess>::Create<RemoteProcess>(&parent, &pid, &options));
			}
			~RemoteProcess()
            {
                TRACE_L1("Destructor for Remote process for %d", _process.Id());
            }

        public:
            virtual uint32_t Id() const
            {
                return (_process.Id());
            }
            virtual enumState State() const
            {
                return (_state);
            }
            virtual void* QueryInterface(const uint32_t id)
            {
                if (id == IRemoteProcess::ID) {
                    AddRef();
                    return (static_cast<IRemoteProcess*>(this));
                }
                else if (_returnedInterface != nullptr) {
                    void* result = _returnedInterface;
                    _returnedInterface = nullptr;
                    return (result);
                }
                return (nullptr);
            }
            virtual void* Instantiate(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version);

            uint32_t WaitState(const uint32_t state, const uint32_t time) const
            {
                return (_state.WaitState(state, time));
            }
            inline bool IsActive() const
            {
                return (_process.IsActive());
            }
            inline uint32_t ExitCode() const
            {
                return (_process.ExitCode());
            }
            void Terminate();
            void Announce(Core::ProxyType<Core::IPCChannel>& channel, const Data::Init& info, void*& implementation);

        protected:
            inline void State(const IRemoteProcess::enumState newState)
            {

                ASSERT(newState != IRemoteProcess::CONSTRUCTED);

                _state.Lock();

                if (_state != newState) {

                    _state = newState;

                    if (_state == IRemoteProcess::DEACTIVATED) {

						_parent->Deactivated(this);

                        if (_channel.IsValid() == true) {
                            _channel.Release();
                        }
                    }
					else if (_state == IRemoteProcess::ACTIVE) {

						_parent->Activated(this);
					}
				}

                _state.Unlock();
            }
            inline void Kill(const bool hardKill)
            {
                return (_process.Kill(hardKill));
            }

        private:
            RemoteProcessMap* _parent;
            Core::Process _process;
            Core::StateTrigger<IRemoteProcess::enumState> _state;
            Core::ProxyType<Core::IPCChannel> _channel;
            void* _returnedInterface;
        };

    private:
        class EXTERNAL RemoteProcessMap {
        private:
            class ClosingInfo {
            private:
                ClosingInfo() = delete;
                ClosingInfo& operator=(const ClosingInfo& RHS) = delete;

                enum enumState {
                    INITIAL,
                    SOFTKILL,
                    HARDKILL
                };

            public:
                ClosingInfo(RemoteProcess* process)
                    : _process(process)
                    , _state(INITIAL)
                {
                    if (_process != nullptr) 
                    {
                        _process->AddRef();
                    }
                }
                ClosingInfo(const ClosingInfo& copy)
                    : _process(copy._process)
                    , _state(copy._state)
                {
                    if (_process != nullptr) 
                    {
                        _process->AddRef();
                    }
                }
                ~ClosingInfo()
                {
                    if (_process != nullptr) 
                    {
						// If we are still active, No time to wait, force kill
						if (_process->IsActive() == true) {
							_process->Kill(true);
						}
                        _process->Release();
                    }
                }

            public:
                uint64_t Timed(const uint64_t scheduledTime)
                {
                    uint64_t result = 0;

                    if (_process->IsActive() == false) {
                        // It is no longer active.... just report, and clear our selves !!!
                        TRACE_L1("Destructed. Closed down nicely [%d].\n", _process->Id());
                        _process->Release();
                        _process = nullptr;
                    }
                    else if (_state == INITIAL) {
                        _state = SOFTKILL;
                        _process->Kill(false);
                        result = Core::Time(scheduledTime).Add(6000).Ticks(); // Next check in 6S
                    }
                    else if (_state == SOFTKILL) {
                        _state = HARDKILL;
                        _process->Kill(true);
                        result = Core::Time(scheduledTime).Add(8000).Ticks(); // Next check in 8S
                    }
                    else {
                        // This should not happen. This is a very stubbern process. Can be killed.
                        ASSERT(false);
                    }

                    return (result);
                }

            private:
                RemoteProcess* _process;
                enumState _state;
            };

        private:
            RemoteProcessMap(const RemoteProcessMap&) = delete;
            RemoteProcessMap& operator=(const RemoteProcessMap&) = delete;

            static constexpr uint32_t DestructionStackSize = 64 * 1024;

        public:
            RemoteProcessMap(Communicator& parent)
                : _adminLock()
                , _processes()
                , _destructor(DestructionStackSize, "ProcessDestructor")
				, _parent(parent)
            {
            }
            virtual ~RemoteProcessMap()
            {
				// All observers should have unregistered before this map get's destroyed !!!
				ASSERT(_observers.size() == 0);

				while (_observers.size() != 0)
				{
					_observers.front()->Release();
					_observers.pop_front();
				}
            }

        public:
			inline void Register(RPC::IRemoteProcess::INotification* sink)
			{
				ASSERT(sink != nullptr);

				if (sink != nullptr) {

					_adminLock.Lock();

					ASSERT(std::find(_observers.begin(), _observers.end(), sink) == _observers.end());

					sink->AddRef();
					_observers.push_back(sink);

					std::map<uint32_t, RemoteProcess* >::iterator index(_processes.begin());

					// Report all Active Processes..
					while (index != _processes.end())
					{
						if (index->second->State() == RemoteProcess::ACTIVE) {
							sink->Activated(&(*(index->second)));
						}
						index++;
					}

					_adminLock.Unlock();
				}
			}
			inline void Unregister(RPC::IRemoteProcess::INotification* sink)
			{
				ASSERT(sink != nullptr);

				if (sink != nullptr) {

					_adminLock.Lock();

					std::list<RPC::IRemoteProcess::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), sink));

					ASSERT(index != _observers.end());

					if (index != _observers.end())
					{
						(*index)->Release();
						_observers.erase(index);
					}

					_adminLock.Unlock();
				}
			}
			inline uint32_t Size() const
            {
                return (static_cast<uint32_t>(_processes.size()));
            }
			inline void* Instance(const string& className, const uint32_t interfaceId, const uint32_t versionId) {
				return (_parent.Instance(className, interfaceId, versionId));
			}

            inline Communicator::RemoteProcess* Create(uint32_t& pid, const Object& instance, const Config& config)
            {
                _adminLock.Lock();

                Communicator::RemoteProcess* result = RemoteProcess::Create(*this, pid, instance, config);

                ASSERT(result != nullptr);

                if (result != nullptr) {

                    // A reference for putting it in the list...
                    result->AddRef();
 
                    // We expect an announce interface message now...
                    _processes.insert(std::pair<uint32_t, RemoteProcess* >(result->Id(), result));
                }

                _adminLock.Unlock();

                return (result);
            }

            inline void Destroy(const uint32_t pid)
            {
                // First do an activity check on all processes registered.
                _adminLock.Lock();

                std::map<uint32_t, Communicator::RemoteProcess* >::iterator index(_processes.find(pid));

                if (index != _processes.end()) {

                    index->second->State(IRemoteProcess::DEACTIVATED);

                    if (index->second->IsActive() == false) {
                        TRACE_L1("CLEAN EXIT of process: %d", pid);
                    }
                    else {
                        Core::Time scheduleTime(Core::Time::Now().Add(3000)); // Check again in 3S...

                        TRACE_L1("Submitting process for closure: %d", pid);
                        _destructor.Schedule(scheduleTime, ClosingInfo(index->second));
                    }

                    index->second->Release();

                    // Release this entry, do not wait till it get's overwritten.
                    _processes.erase(index);
                }

                _adminLock.Unlock();
            }

            Communicator::RemoteProcess* Process(const uint32_t id)
            {
                Communicator::RemoteProcess* result = nullptr;

                _adminLock.Lock();

                std::map<uint32_t, Communicator::RemoteProcess* >::iterator index(_processes.find(id));

                if (index != _processes.end()) {
                    result = index->second;
                    result->AddRef();
                }

                _adminLock.Unlock();

                return (result);
            }

            inline void Processes(std::list<uint32_t>& pidList) const
            {
                // First do an activity check on all processes registered.
                _adminLock.Lock();

                std::map<uint32_t, Communicator::RemoteProcess* >::const_iterator index(_processes.begin());

                while (index != _processes.end()) {
                    pidList.push_back(index->first);
                    index++;
                }

                _adminLock.Unlock();
            }
            uint32_t Announce(Core::ProxyType<Core::IPCChannel>& channel, const Data::Init& info, void*& implementation)
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                if (channel.IsValid() == true) {

                    _adminLock.Lock();

                    std::map<uint32_t, Communicator::RemoteProcess* >::iterator index(_processes.find(info.ExchangeId()));

					if (index == _processes.end()) {
						// This is an announce message from a process that wasn't created by us. So typically this is 
						// An RPC client reaching out to an RPC server. The RPCServer does not spawn processes it just
						// listens for clients requesting service.
						Communicator::RemoteProcess* remoteProcess = RemoteProcess::Create(*this, info.ExchangeId(), channel);

						ASSERT(remoteProcess != nullptr);

						// Add ref is done during the creation, no need to take another reference unless we also would release it after 
						// insertion :-)
						auto newElement = _processes.insert(std::pair<uint32_t, Communicator::RemoteProcess* >(info.ExchangeId(), remoteProcess));

						index = newElement.first;
					}

					ASSERT (index != _processes.end());

					if (implementation == nullptr) {
					
						// See if we have something we can return right away, if it has been requested..
						implementation = Instance(info.ClassName(), info.InterfaceId(), info.VersionId());
					}
					else {
						index->second->Announce(channel, info, implementation);
                        index->second->State(IRemoteProcess::ACTIVE);

                    } 

					result = Core::ERROR_NONE;

                    _adminLock.Unlock();
                }

                return (result);
            }

			void Activated(RPC::IRemoteProcess* process)
			{
				_adminLock.Lock();

				std::list<RPC::IRemoteProcess::INotification*>::iterator index(_observers.begin());

				while (index != _observers.end())
				{
					(*index)->Activated(process);
					index++;
				}

				_adminLock.Unlock();
			}
			void Deactivated(RPC::IRemoteProcess* process)
			{
				_adminLock.Lock();

				std::list<RPC::IRemoteProcess::INotification*>::iterator index(_observers.begin());

				while (index != _observers.end())
				{
					(*index)->Deactivated(process);
					index++;
				}

				_adminLock.Unlock();
			}

        private:
            mutable Core::CriticalSection _adminLock;
            std::map<uint32_t, Communicator::RemoteProcess* > _processes;
            std::list<RPC::IRemoteProcess::INotification*> _observers;
            Core::TimerType<ClosingInfo> _destructor;
			Communicator& _parent;
        };
        class EXTERNAL ProcessChannelLink {
        private:
            ProcessChannelLink() = delete;
            ProcessChannelLink(const ProcessChannelLink&) = delete;
            ProcessChannelLink& operator=(const ProcessChannelLink&) = delete;

        public:
            ProcessChannelLink(Core::IPCChannelType<Core::SocketPort, ProcessChannelLink>* channel)
                : _channel(*channel)
                , _processMap(nullptr)
                , _pid(0)
            {
                // We are a composit of the Channel, no need (and do not for cyclic references) not maintain a reference...
                ASSERT(channel != nullptr);
            }
            ~ProcessChannelLink()
            {
            }

        public:
            inline bool IsValid() const
            {
                return (_processMap != nullptr);
            }
            void Link(RemoteProcessMap& processMap, const uint32_t pid)
            {
                _processMap = &processMap;
                _pid = pid;
			}
            uint32_t Pid() const
            {
                return (_pid);
            }

            void StateChange()
            {
				if ((_channel.Source().IsOpen() == false) && (_pid != 0)) {

                    ASSERT(_processMap != nullptr);

                    // Seems there is not much we can do, time to kill the process, we lost a connection with it.
                    TRACE_L1("Lost connection on process [%d]. Closing the process.\n", _pid);
                    _processMap->Destroy(_pid);
                    _pid = 0;
                }
            }

        private:
            // Non ref-counted reference to our parent, of which we are a composit :-)
            Core::IPCChannelType<Core::SocketPort, ProcessChannelLink>& _channel;
            RemoteProcessMap* _processMap;
            uint32_t _pid;
        };
        class EXTERNAL ProcessChannelServer : public Core::IPCChannelServerType<ProcessChannelLink, true> {
        private:
            ProcessChannelServer(const ProcessChannelServer&) = delete;
            ProcessChannelServer& operator=(const ProcessChannelServer&) = delete;

            typedef Core::IPCChannelServerType<ProcessChannelLink, true> BaseClass;
            typedef Core::IPCChannelType<Core::SocketPort, ProcessChannelLink> Client;

            class EXTERNAL InterfaceAnnounceHandler : public Core::IPCServerType<AnnounceMessage> {
            private:
                InterfaceAnnounceHandler() = delete;
                InterfaceAnnounceHandler(const InterfaceAnnounceHandler&) = delete;
                InterfaceAnnounceHandler& operator=(const InterfaceAnnounceHandler&) = delete;

            public:
                InterfaceAnnounceHandler(ProcessChannelServer* parent)
                    : _parent(*parent)
                {

                    ASSERT(parent != nullptr);
                }

                virtual ~InterfaceAnnounceHandler()
                {
                }

            public:
                virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<AnnounceMessage>& data)
                {
					void* result = data->Parameters().Implementation();

                    // Anounce the interface as completed
                    string jsonDefaultCategories;
                    Trace::TraceUnit::Instance().GetDefaultCategoriesJson(jsonDefaultCategories);
                    _parent.Announce(channel, data->Parameters(), result);
                    data->Response().Set(result, _parent.ProxyStubPath(), jsonDefaultCategories);

                    // We are done, report completion
                    Core::ProxyType<Core::IIPC> baseMessage(Core::proxy_cast<Core::IIPC>(data));
                    channel.ReportResponse(baseMessage);
                }

            private:
                ProcessChannelServer& _parent;
            };
			class EXTERNAL InterfaceObjectHandler : public Core::IPCServerType<ObjectMessage>
			{
			private:
				InterfaceObjectHandler() = delete;
				InterfaceObjectHandler(const InterfaceObjectHandler &) = delete;
				InterfaceObjectHandler & operator=(const InterfaceObjectHandler &) = delete;

			public:
				InterfaceObjectHandler(RemoteProcessMap* handler)
					: _handler(*handler)
				{
					ASSERT(handler != nullptr);
				}
				~InterfaceObjectHandler()
				{
				}

			public:
				virtual void Procedure(Core::IPCChannel & channel, Core::ProxyType<RPC::ObjectMessage> & data)
				{
					// Oke, see if we can reference count the IPCChannel
					Core::ProxyType<Core::IPCChannel> refChannel(channel);

					ASSERT(refChannel.IsValid());

					if (refChannel.IsValid() == true)
					{
						void* implementation = _handler.Instance(data->Parameters().ClassName(), data->Parameters().InterfaceId(), data->Parameters().VersionId());

						if (implementation != nullptr)
						{
							// Allright, respond with the interface.
							data->Response().Value(implementation);
						}
						else
						{
							// Allright, respond with the interface.
							data->Response().Value(nullptr);
						}
					}

					Core::ProxyType<Core::IIPC> returnValue(data);
					channel.ReportResponse(returnValue);
				}

			private:
				RemoteProcessMap& _handler;
			};

        public:
			#ifdef __WIN32__ 
			#pragma warning( disable : 4355 )
			#endif
			ProcessChannelServer(const Core::NodeId& remoteNode, RemoteProcessMap& processes, const Core::ProxyType<Core::IIPCServer>& handler, const string& proxyStubPath)
                : BaseClass(remoteNode, CommunicationBufferSize)
				, _proxyStubPath(proxyStubPath)
                , _processes(processes)
                , _interfaceAnnounceHandler(Core::ProxyType<InterfaceAnnounceHandler>::Create(this))
				, _interfaceObjectHandler(Core::ProxyType<InterfaceObjectHandler>::Create(&_processes))
				, _interfaceMessageHandler(handler)
            {
                BaseClass::Register(_interfaceAnnounceHandler);
				BaseClass::Register(_interfaceObjectHandler);
				BaseClass::Register(handler);
            }
			#ifdef __WIN32__ 
			#pragma warning( default : 4355 )
			#endif

			~ProcessChannelServer()
            {
                BaseClass::Unregister(_interfaceAnnounceHandler);
				BaseClass::Unregister(_interfaceObjectHandler);
				BaseClass::Unregister(_interfaceMessageHandler);
            }

		public:
			inline const string& ProxyStubPath() const {
				return (_proxyStubPath);
			}

        private:
            inline uint32_t Announce(Core::IPCChannel& channel, const Data::Init& info, void*& implementation)
            {
                Core::ProxyType<Core::IPCChannel> baseChannel(channel);

                ASSERT(baseChannel.IsValid() == true);

                uint32_t result = _processes.Announce(baseChannel, info, implementation);

                if (result == Core::ERROR_NONE) {

                    // We are in business, register the process with this channel.
                    ASSERT(dynamic_cast<Client*>(&channel) != nullptr);

                    Client& client(static_cast<Client&>(channel));

                    client.Extension().Link(_processes, info.ExchangeId());
                }

                return (result);
            }

        private:
			const string _proxyStubPath;
            RemoteProcessMap& _processes;
            Core::ProxyType<Core::IIPCServer> _interfaceAnnounceHandler; // IPCInterfaceAnnounce
            Core::ProxyType<Core::IIPCServer> _interfaceMessageHandler; //IPCInterfaceMessage
			Core::ProxyType<Core::IIPCServer> _interfaceObjectHandler;
        };

    private:
        Communicator() = delete;
        Communicator(const Communicator&) = delete;
        Communicator& operator=(const Communicator&) = delete;

    public:
        Communicator(const Core::NodeId& node, const Core::ProxyType<Core::IIPCServer>& handler, const string& proxyStubPath);
        virtual ~Communicator();

    public:
		inline bool IsListening() const {
			return (_ipcServer.IsListening());
		}
	inline uint32_t Open(const uint32_t waitTime) {
		return (_ipcServer.Open(waitTime));
	}
	inline uint32_t Close(const uint32_t waitTime) {
		return (_ipcServer.Close(waitTime));
	}
	template <typename ACTUALELEMENT>
	inline void CreateFactory(const uint32_t initialSize)
	{
		_ipcServer.CreateFactory<ACTUALELEMENT>(initialSize);
	}
	template <typename ACTUALELEMENT>
	inline void DestroyFactory()
	{
		_ipcServer.DestroyFactory<ACTUALELEMENT>();
	}
	inline void Register(const Core::ProxyType<Core::IIPCServer>& handler)
	{
		_ipcServer.Register(handler);
	}
	inline void Unregister(const Core::ProxyType<Core::IIPCServer>& handler)
	{
		_ipcServer.Unregister(handler);
	}
	inline const string& Connector() const {
		return (_ipcServer.Connector());
	}
	inline void Register(RPC::IRemoteProcess::INotification* sink)
	{
		_processMap.Register(sink);
	}
	inline void Unregister(RPC::IRemoteProcess::INotification* sink)
	{
		_processMap.Unregister(sink);
	}
	inline void Processes(std::list<uint32_t>& pidList) const
        {
            _processMap.Processes(pidList);
        }
        inline Communicator::RemoteProcess* Process(const uint32_t id)
        {
            return (_processMap.Process(id));
        }
		inline Communicator::RemoteProcess* Create(uint32_t& pid, const Object& instance, const Config& config) {
			return (_processMap.Create(pid, instance, config));
		}

        // Use this method with care. It goes beyond the refence counting taking place in the object.
        // This method kills the process without any considerations!!!!
        inline void Destroy(const uint32_t& pid)
        {
            _processMap.Destroy(pid);
        }

	private:
		virtual void* Instance (const string& /* className */, const uint32_t /* interfaceId */, const uint32_t /* version */) {
			return (nullptr);
		}

    private:
        RemoteProcessMap _processMap;
        ProcessChannelServer _ipcServer;
        Core::ProxyType<Core::IIPCServer> _stubHandler;
    };

    class EXTERNAL CommunicatorClient : public Core::IPCChannelClientType<Core::Void, false, true>, public Core::IDispatchType<Core::IIPC> {
    private:
		CommunicatorClient() = delete;
		CommunicatorClient(const CommunicatorClient&) = delete;
        CommunicatorClient& operator=(const CommunicatorClient&) = delete;

        typedef Core::IPCChannelClientType<Core::Void, false, true> BaseClass;

    public:
        CommunicatorClient(const Core::NodeId& remoteNode);
        ~CommunicatorClient();

    public:
		template <typename INTERFACE>
		inline INTERFACE* Create(const string& className, const uint32_t version = static_cast<uint32_t>(~0), const uint32_t waitTime = CommunicationTimeOut)
        {
            return (static_cast<INTERFACE*>(Create(waitTime, className, INTERFACE::ID, version)));
        }

		template <typename INTERFACE>
		static inline INTERFACE* Create(const Core::NodeId& remoteNode, const string& className, const uint32_t version = static_cast<uint32_t>(~0), const uint32_t waitTime = CommunicationTimeOut) {
			INTERFACE* result = nullptr;
			Core::ProxyType<RPC::CommunicatorClient> client(Core::ProxyType<RPC::CommunicatorClient>::Create(remoteNode));

			if ((client.IsValid() == true) && (client->Open(waitTime, className, INTERFACE::ID, version) == Core::ERROR_NONE))
			{
				// Oke we could open the channel, lets get the interface
				result = client->WaitForCompletion<INTERFACE>(waitTime);
			}

			return (result);
		}

		// Open and request an interface from the other side on the announce message (Any RPC client uses this)
        uint32_t Open(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version);

		// Open and offer the requested interface (Applicable if the WPEProcess starts the RPCClient) 
		uint32_t Open(const uint32_t waitTime, const uint32_t interfaceId, void* implementation);

		uint32_t Close(const uint32_t waitTime);

		// If the Open, with a request was made, this method waits for the requested interface.
		template <typename INTERFACE>
		inline INTERFACE* WaitForCompletion(const uint32_t waitTime)
		{
			INTERFACE* result = nullptr;

			ASSERT(_announceMessage->Parameters().InterfaceId() == INTERFACE::ID);
			ASSERT(_announceMessage->Parameters().Implementation() == nullptr);

			// Lock event until Dispatch() sets it.
			if (_announceEvent.Lock(waitTime) == Core::ERROR_NONE) {

				void* implementation(_announceMessage->Response().Implementation());

				ASSERT(implementation != nullptr);

				if (implementation != nullptr) {
					Core::ProxyType<Core::IPCChannel> baseChannel(*this);

					ASSERT(baseChannel.IsValid() == true);

					result = Administrator::Instance().CreateProxy<INTERFACE>(baseChannel, static_cast<INTERFACE*>(implementation), false, true);
				}
			}

			return (result);
		}

    private:
		void* Create(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version = static_cast<uint32_t>(~0));

		virtual void StateChange();
		virtual void Dispatch(Core::IIPC& element);

	private:
		Core::ProxyType<RPC::AnnounceMessage> _announceMessage;
		Core::Event _announceEvent;
	};
}
}

#endif // __COM_PROCESSLAUNCH_H
