// Reflow Controller Dashboard JavaScript
class ReflowDashboard {
    constructor() {
        this.temperatureChart = null;
        this.updateInterval = 1000;
        this.init();
    }
    
    init() {
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', () => this.setup());
        } else {
            this.setup();
        }
    }
    
    setup() {
        this.initializeChart();
        this.startDataUpdates();
    }
    
    initializeChart() {
        this.temperatureChart = Highcharts.chart('temperatureChart', {
            chart: {
                type: 'spline',
                animation: false,
                backgroundColor: 'transparent'
            },
            title: { text: null },
            xAxis: {
                type: 'datetime',
                title: { text: 'Time' },
                dateTimeLabelFormats: {
                    second: '%H:%M:%S',
                    minute: '%H:%M',
                    hour: '%H:%M',
                    day: '%m-%d',
                    week: '%m-%d',
                    month: '%Y-%m',
                    year: '%Y'
                }
            },
            yAxis: {
                title: { text: 'Temperature (&deg;C)' },
                plotLines: [{
                    value: 0,
                    width: 1,
                    color: '#808080'
                }]
            },
            tooltip: {
                formatter: function() {
                    return '<b>Temperature</b><br/>' +
                           Highcharts.dateFormat('%H:%M:%S', this.x) + '<br/>' +
                           '<span style="color: #e74c3c; font-weight: bold;">' +
                           this.y.toFixed(1) + ' &deg;C</span>';
                }
            },
            legend: { enabled: false },
            plotOptions: {
                spline: {
                    marker: { enabled: false },
                    lineWidth: 3,
                    color: '#e74c3c'
                }
            },
            series: [{
                name: 'Temperature',
                data: []
            }],
            credits: { enabled: false }
        });
    }
    
    updateData() {
        fetch('/data')
            .then(response => response.json())
            .then(data => {
                // Update temperature chart and display
                if (data.temperature_data && data.temperature_data.length > 0) {
                    this.temperatureChart.series[0].setData(data.temperature_data, true);
                    
                    const latest = data.temperature_data[data.temperature_data.length - 1];
                    if (latest) {
                        document.getElementById('currentTemp').innerHTML = latest[1].toFixed(1) + ' &deg;C';
                    }
                }
                
                // Update switch status
                const btn = document.getElementById('reflowSwitchBtn');
                const stateSpan = document.getElementById('switchState');
                
                if (btn && stateSpan) {
                    if (data.switch_state) {
                        btn.classList.add('on');
                        stateSpan.textContent = 'ON';
                    } else {
                        btn.classList.remove('on');
                        stateSpan.textContent = 'OFF';
                    }
                }
            })
            .catch(error => {
                console.error('Error fetching data:', error);
            });
    }
    
    startDataUpdates() {
        this.updateData();
        
        // Update both temperature and switch data together
        setInterval(() => this.updateData(), this.updateInterval);
    }
}

// Initialize dashboard
new ReflowDashboard();

// Switch control function
function toggleReflowSwitch() {
    const btn = document.getElementById('reflowSwitchBtn');
    const stateSpan = document.getElementById('switchState');
    const currentState = btn.classList.contains('on');
    const newState = !currentState;
    
    fetch('/switch_control', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ state: newState })
    })
    .then(response => response.json())
    .then(data => {
        if (data.state) {
            btn.classList.add('on');
            stateSpan.textContent = 'ON';
        } else {
            btn.classList.remove('on');
            stateSpan.textContent = 'OFF';
        }
    })
    .catch(error => {
        console.error('Error controlling switch:', error);
    });
}
